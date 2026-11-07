import shlex
import subprocess
import time
from typing import Any, cast

# All passwords and images here are disposable VM fixtures.
subprocess.run(["truncate", "-s", "256M", str(machine.state_dir / "sdcard.raw")], check=True)

def boot(wrong_first=False):
    machine.start()
    machine.wait_for_console_text("Starting password query on /dev/tty1", timeout=60)
    if wrong_first:
        machine.send_chars("differentpass\n")
        machine.wait_for_console_text("Passphrase incorrect", timeout=20)
        # The repeated prompt has no newline; the agent's log confirms the
        # next VT keyboard query is active without waiting on line buffering.
        machine.wait_for_console_text("Starting password query on /dev/tty1", timeout=20)
    machine.send_chars("testpass123\n")
    machine.wait_for_unit("multi-user.target")


def key():
    return machine.succeed("keyctl search @u user nixos:sd-unlock").strip()


def no_key():
    machine.fail("keyctl search @u user nixos:sd-unlock")


def cache(values):
    machine.succeed("keyctl link @u @s")
    encoded = "\\000".join(values)
    machine.succeed("printf " + shlex.quote(encoded) + " | keyctl padd user cryptsetup @u >/dev/null")


def export_password(values=("testpass123",), measured=False):
    cache(values)
    options = "password-cache=read-only,x-nixos.sd-passphrase"
    if measured:
        options += ",tpm2-measure-pcr=15"
    machine.succeed('timeout 10 "$(cat /etc/test-cryptsetup)" attach proof /dev/vdb - ' + shlex.quote(options) + ' </dev/null 2>&1')
    machine.succeed("cryptsetup close proof")
    return key()


def worker():
    began = time.monotonic()
    machine.succeed("touch /var/lib/test-auto-unlock; systemctl start nixos-sd-unlock")
    assert time.monotonic() - began < 3, "card work delayed service startup"
    machine.wait_until_succeeds("test $(systemctl show nixos-sd-unlock -p ActiveState --value) = inactive", timeout=26)
    machine.succeed("test $(systemctl show nixos-sd-unlock -p Result --value) = success")
    no_key()
    machine.fail("test -e /dev/mapper/luks-$(cryptsetup luksUUID /dev/vdb 2>/dev/null)")


def unmounted(device="/dev/mmcblk0"):
    machine.fail("findmnt /run/media/carduser/SD")
    machine.fail(f"test -e /dev/mapper/luks-$(cryptsetup luksUUID {device})")


def close_card(device="/dev/mmcblk0"):
    machine.succeed(f"umount /run/media/carduser/SD; cryptsetup close luks-$(cryptsetup luksUUID {device})")


with subtest("real root unlock exports a private key across switch-root"):
    boot()
    machine.succeed("test $(cat /sys/class/block/mmcblk0/device/type) = SD")
    machine.succeed("findmnt -n -o SOURCE / | grep /dev/mapper/sdtestroot")
    serial = key()
    machine.succeed(f"keyctl rdescribe {serial} | grep '3f0000.*nixos:sd-unlock'")
    machine.fail(f"runuser -u carduser -- keyctl pipe {serial}")
    machine.fail(f"runuser -u carduser -- keyctl update {serial} replaced")
    machine.fail(f"runuser -u carduser -- unshare -Ur keyctl pipe {serial}")
    machine.succeed("umask 077; printf testpass123 > /run/fixture-pass; printf differentpass > /run/fixture-other")
    machine.succeed(f"keyctl pipe {serial} | cmp - /run/fixture-pass")
    machine.succeed("mkfs.ext4 -q -L SD /dev/mmcblk0")
    worker()
    machine.fail("findmnt /run/media/carduser/SD")

with subtest("only a verified root candidate is exported"):
    machine.succeed("cryptsetup luksFormat --batch-mode --pbkdf pbkdf2 --iter-time 1 --key-file /run/fixture-pass /dev/vdb")
    serial = export_password(("differentpass", "testpass123"))
    machine.succeed(f"keyctl pipe {serial} | cmp - /run/fixture-pass")
    machine.succeed(f"keyctl revoke {serial}")
    # The measured-volume-key branch must preserve exactly the same provenance.
    serial = export_password(measured=True)
    machine.succeed(f"keyctl pipe {serial} | cmp - /run/fixture-pass")
    machine.succeed(f"keyctl revoke {serial}")
    machine.succeed('"$(cat /etc/test-cryptsetup)" attach proof /dev/vdb /run/fixture-pass x-nixos.sd-passphrase </dev/null 2>&1')
    no_key()
    # Already-active return cannot make a supplied candidate authoritative.
    machine.succeed('"$(cat /etc/test-cryptsetup)" attach proof /dev/vdb - x-nixos.sd-passphrase </dev/null 2>&1')
    no_key()
    machine.succeed("cryptsetup close proof")

with subtest("absent SD is optional and insertion later does not retry"):
    machine.succeed("basename $(realpath /sys/class/mmc_host/mmc0/device) > /run/sd-controller")
    machine.succeed("cat /run/sd-controller > /sys/bus/pci/drivers/sdhci-pci/unbind")
    machine.wait_until_fails("test -e /sys/class/block/mmcblk0")
    export_password()
    worker()
    machine.succeed("cat /run/sd-controller > /sys/bus/pci/drivers/sdhci-pci/bind")
    machine.wait_until_succeeds("test -e /dev/mmcblk0")
    no_key()

with subtest("default LUKS2 Argon2 card unlocks and mounts for the configured username"):
    machine.succeed("cryptsetup luksFormat --batch-mode --key-file /run/fixture-pass /dev/mmcblk0", timeout=90)
    machine.succeed("cryptsetup open --key-file /run/fixture-pass /dev/mmcblk0 format-card")
    machine.succeed("mkfs.ext4 -q -L SD /dev/mapper/format-card; mkdir -p /mnt/card; mount /dev/mapper/format-card /mnt/card; chown carduser /mnt/card; umount /mnt/card; cryptsetup close format-card")
    export_password()
    worker()
    machine.succeed("findmnt /run/media/carduser/SD; runuser -u carduser -- touch /run/media/carduser/SD/game-save")
    machine.succeed("test $(stat -c %u /run/media/carduser/SD/game-save) = 1234")
    close_card()
    worker()  # no key remains to unlock it again
    unmounted()

with subtest("wrong SD password skips, including rejected root candidates that match the card"):
    machine.succeed("cryptsetup luksChangeKey --key-file /run/fixture-pass /dev/mmcblk0 /run/fixture-other", timeout=90)
    export_password(("differentpass", "testpass123"))
    worker()
    unmounted()
    # Normal UDisks/manual unlocking still works with the card's own password.
    machine.succeed("udisksctl unlock --no-user-interaction -b /dev/mmcblk0 --key-file /run/fixture-other")
    machine.succeed("udisksctl lock --no-user-interaction -b /dev/mmcblk0")
    machine.succeed("cryptsetup luksChangeKey --key-file /run/fixture-other /dev/mmcblk0 /run/fixture-pass", timeout=90)

with subtest("expired or malformed handoff never attempts the card"):
    serial = export_password()
    machine.succeed(f"keyctl timeout {serial} 1")
    time.sleep(2)
    worker()
    unmounted()
    serial = export_password()
    machine.succeed(f"keyctl setperm {serial} 0x3f3f0000")
    worker()
    unmounted()

with subtest("TPM-only root unlock does not export a password"):
    machine.wait_until_succeeds("test -e /dev/tpmrm0")
    machine.succeed("systemd-cryptenroll --unlock-key-file=/run/fixture-pass --tpm2-device=auto --tpm2-pcrs=7 /dev/vdb", timeout=60)
    machine.succeed('"$(cat /etc/test-cryptsetup)" attach proof /dev/vdb - tpm2-device=auto,x-nixos.sd-passphrase </dev/null 2>&1')
    no_key()
    machine.succeed("cryptsetup close proof")
    worker()
    unmounted()
    machine.succeed("systemd-cryptenroll --wipe-slot=tpm2 /dev/vdb")

with subtest("a stalled cryptographic worker is killed without blocking the session"):
    export_password()
    began = time.monotonic()
    machine.succeed("systemctl start nixos-sd-unlock; parent=$(systemctl show nixos-sd-unlock -p MainPID --value); child=; for i in $(seq 1 100); do child=$(pgrep -P $parent || true); if test -n \"$child\"; then kill -STOP $child; break; fi; sleep .01; done; test -n \"$child\"; echo $child > /run/stopped-worker; systemctl is-active multi-user.target")
    assert time.monotonic() - began < 3
    machine.succeed("ps -o stat= -p $(cat /run/stopped-worker) | grep T")
    machine.fail("grep -aF testpass123 /proc/$(cat /run/stopped-worker)/cmdline /proc/$(cat /run/stopped-worker)/environ")
    machine.succeed("test $(systemctl show nixos-sd-unlock -p MemorySwapMax --value) = 0; test $(systemctl show nixos-sd-unlock -p LimitCORE --value) = 0")
    no_key()
    machine.fail("journalctl -b -u nixos-sd-unlock | grep -F testpass123")
    machine.wait_until_succeeds("test $(systemctl show nixos-sd-unlock -p ActiveState --value) = inactive", timeout=26)
    assert 18 <= time.monotonic() - began < 26
    machine.fail("kill -0 $(cat /run/stopped-worker)")
    unmounted()

with subtest("real SD read failures are optional"):
    machine.succeed("blockdev --flushbufs /dev/mmcblk0; echo 3 > /proc/sys/vm/drop_caches")
    cast(Any, machine.qmp_client).send("blockdev-add", {"driver": "blkdebug", "node-name": "sd-errors", "image": "sdfile", "inject-error": [{"event": "read_aio", "errno": 5, "immediately": True}]})
    cast(Any, machine.qmp_client).send("blockdev-reopen", {"options": [{"driver": "raw", "node-name": "sdcard", "file": "sd-errors"}]})
    export_password()
    worker()
    cast(Any, machine.qmp_client).send("blockdev-reopen", {"options": [{"driver": "raw", "node-name": "sdcard", "file": "sdfile"}]})
    cast(Any, machine.qmp_client).send("blockdev-del", {"node-name": "sd-errors"})
    unmounted()

with subtest("a rejected then successful root password automatically unlocks the card on reboot"):
    machine.shutdown()
    boot(wrong_first=True)
    machine.wait_until_succeeds("test -e /run/media/carduser/SD/game-save", timeout=26)
    machine.wait_until_succeeds("test $(systemctl show nixos-sd-unlock -p ActiveState --value) = inactive", timeout=26)
    machine.succeed("test $(systemctl show nixos-sd-unlock -p Result --value) = success")
    no_key()
    machine.succeed("systemctl is-active multi-user.target")

with subtest("a partitioned native SD is discovered without a device registry"):
    close_card()
    machine.succeed("umask 077; printf testpass123 > /run/fixture-pass")
    machine.succeed("wipefs -a /dev/mmcblk0; parted -s /dev/mmcblk0 mklabel gpt mkpart primary 1MiB 100%; udevadm settle")
    machine.wait_until_succeeds("test -b /dev/mmcblk0p1")
    machine.succeed("cryptsetup luksFormat --batch-mode --pbkdf pbkdf2 --iter-time 1 --key-file /run/fixture-pass /dev/mmcblk0p1")
    machine.succeed("cryptsetup open --key-file /run/fixture-pass /dev/mmcblk0p1 format-card; mkfs.ext4 -q -L SD /dev/mapper/format-card; cryptsetup close format-card")
    export_password()
    worker()
    machine.succeed("findmnt /run/media/carduser/SD")
    close_card("/dev/mmcblk0p1")
    export_password()
    machine.succeed("nixos-sd-unlock nonexistent-root carduser")
    no_key()
    unmounted("/dev/mmcblk0p1")

with subtest("a TPM-only root boot reaches the session and leaves the SD locked"):
    machine.succeed("systemd-cryptenroll --unlock-key-file=/run/fixture-pass --tpm2-device=auto --tpm2-pcrs=7 /dev/vda", timeout=60)
    machine.shutdown()
    machine.start()
    # No console input: the actual root must be opened by its software TPM.
    machine.wait_for_unit("multi-user.target", timeout=60)
    machine.succeed("findmnt -n -o SOURCE / | grep /dev/mapper/sdtestroot")
    machine.wait_until_succeeds("test $(systemctl show nixos-sd-unlock -p ActiveState --value) = inactive", timeout=26)
    machine.succeed("test $(systemctl show nixos-sd-unlock -p Result --value) = success")
    no_key()
    unmounted("/dev/mmcblk0p1")
