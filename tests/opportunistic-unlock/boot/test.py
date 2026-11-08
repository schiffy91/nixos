import shlex
import subprocess
import time
from typing import TYPE_CHECKING, Any, cast

if TYPE_CHECKING:
    # Editor declarations only; the NixOS test driver supplies these globals.
    from nixos_test import machine, subtest  # ty: ignore[unresolved-import]

# Every password and disk is a disposable VM fixture.
for image in ("sdcard.raw", "usb.raw", "nvme.raw"):
    subprocess.run(["truncate", "-s", "256M", str(machine.state_dir / image)], check=True)

SERVICE = "nixos-opportunistic-unlock"
KEY = "nixos:opportunistic-unlock"
SD = "/dev/mmcblk0"
USB = "/dev/sda"
INTERNAL = "/dev/vdc"
NVME = "/dev/nvme0n1p1"
DEVICES = (SD, USB, INTERNAL, NVME)


def boot(wrong_first=False):
    machine.start()
    machine.wait_for_console_text("Starting password query on /dev/tty1", timeout=60)
    if wrong_first:
        machine.send_chars("differentpass\n")
        machine.wait_for_console_text("Passphrase incorrect", timeout=20)
        machine.wait_for_console_text("Starting password query on /dev/tty1", timeout=20)
    machine.send_chars("testpass123\n")
    machine.wait_for_unit("multi-user.target")


def key():
    return machine.succeed(f"keyctl search @u user {KEY}").strip()


def no_key():
    machine.fail(f"keyctl search @u user {KEY}")


def cache(values):
    machine.succeed("keyctl link @u @s")
    encoded = "\\000".join(values)
    machine.succeed("printf " + shlex.quote(encoded) + " | keyctl padd user cryptsetup @u >/dev/null")


def export_password(values=("testpass123",), measured=False):
    machine.succeed("if cryptsetup status already-open-proof >/dev/null; then cryptsetup close already-open-proof; fi")
    cache(values)
    options = "password-cache=read-only,x-nixos.opportunistic-passphrase"
    if measured:
        options += ",tpm2-measure-pcr=15"
    machine.succeed('timeout 10 "$(cat /etc/test-cryptsetup)" attach already-open-proof /dev/vdb - ' + shlex.quote(options) + ' </dev/null 2>&1')
    # Leave this backing device open under an arbitrary name. It must never
    # receive a second mapping from the inventory worker.
    return key()


def mappings(device):
    return machine.succeed(f"lsblk -nrpo NAME,TYPE {device} | awk '$2 == \"crypt\" {{print $1}}'").splitlines()


def assert_closed(device):
    assert mappings(device) == [], f"unexpected mapping for {device}"


def assert_open(device):
    found = mappings(device)
    assert len(found) == 1, f"expected one mapping for {device}, got {found}"
    machine.succeed("cryptsetup status " + shlex.quote(found[0]))
    machine.fail("findmnt -rn --source " + shlex.quote(found[0]))
    return found[0]


def close_devices():
    for device in DEVICES:
        for mapping in mappings(device):
            machine.succeed("cryptsetup close " + shlex.quote(mapping))


def wait_worker():
    machine.wait_until_succeeds(f"test $(systemctl show {SERVICE} -p ActiveState --value) = inactive", timeout=26)
    machine.succeed(f"test $(systemctl show {SERVICE} -p Result --value) = success")
    no_key()
    assert len(mappings("/dev/vda")) == 1, "root backing was opened again"
    if machine.execute("cryptsetup status already-open-proof >/dev/null")[0] == 0:
        assert len(mappings("/dev/vdb")) == 1, "already-open backing received a second mapping"


def worker():
    began = time.monotonic()
    machine.succeed(f"touch /var/lib/test-auto-unlock; systemctl start {SERVICE}")
    assert time.monotonic() - began < 3, "optional volume work delayed service startup"
    wait_worker()


def format_luks(device, kind="luks2", keyfile="/run/fixture-pass", argon=False, cipher=None):
    machine.succeed(f"wipefs -a {device}")
    options = "--pbkdf argon2id --pbkdf-memory 65536 --pbkdf-parallel 2 --pbkdf-force-iterations 4" if argon else "--pbkdf pbkdf2 --iter-time 1"
    if cipher:
        options += " --cipher " + cipher + " --key-size 256"
    machine.succeed(f"cryptsetup luksFormat --batch-mode --type {kind} {options} --key-file {keyfile} {device}", timeout=90)
    machine.succeed(f"cryptsetup open --key-file {keyfile} {device} format-volume; mkfs.ext4 -q -L OPTIONAL /dev/mapper/format-volume; cryptsetup close format-volume", timeout=90)


def start_stopped_candidate():
    # Sorted inventory starts with mmcblk0; its Argon2 work gives us time to
    # stop that per-device child while the second worker slot remains runnable.
    export_password()
    machine.succeed(f"systemctl start {SERVICE}; parent=$(systemctl show {SERVICE} -p MainPID --value); child=; for i in $(seq 1 500); do child=$(pgrep -P $parent | sort -n | head -n1); if test -n \"$child\" && kill -STOP $child; then break; fi; sleep .001; done; test -n \"$child\"; echo $child > /run/stopped-worker; systemctl is-active multi-user.target")
    machine.succeed("ps -o stat= -p $(cat /run/stopped-worker) | grep T")
    no_key()


with subtest("real root unlock exports an owner-only key across switch-root"):
    boot()
    for device in (SD, USB, INTERNAL, "/dev/nvme0n1"):
        machine.wait_until_succeeds(f"test -b {device}")
    machine.succeed("test $(cat /sys/class/block/mmcblk0/device/type) = SD")
    machine.succeed("findmnt -n -o SOURCE / | grep /dev/mapper/testroot")
    serial = key()
    machine.succeed(f"keyctl rdescribe {serial} | grep '3f0000.*{KEY}'")
    ttl = machine.succeed(f"awk '$1 == \"{int(serial):08x}\" {{print $4}}' /proc/keys").strip()
    assert ttl.endswith("s") and 0 < int(ttl[:-1]) <= 45, f"handoff has no bounded expiry: {ttl}"
    machine.fail(f"runuser -u volumeuser -- keyctl pipe {serial}")
    machine.fail(f"runuser -u volumeuser -- keyctl update {serial} replaced")
    machine.fail(f"runuser -u volumeuser -- unshare -Ur keyctl pipe {serial}")
    machine.succeed("umask 077; printf testpass123 > /run/fixture-pass; printf differentpass > /run/fixture-other")
    machine.succeed(f"keyctl pipe {serial} | cmp - /run/fixture-pass")
    machine.succeed(f"mkfs.ext4 -q {SD}")
    worker()
    assert_closed(SD)
    machine.fail("systemctl is-active udisks2")
    machine.succeed("parted -s /dev/nvme0n1 mklabel gpt mkpart primary 1MiB 100%; udevadm settle")
    machine.wait_until_succeeds(f"test -b {NVME}")

with subtest("only successful passphrase activation exports a handoff"):
    machine.succeed("cryptsetup luksFormat --batch-mode --pbkdf pbkdf2 --iter-time 1 --key-file /run/fixture-pass /dev/vdb")
    serial = export_password(("differentpass", "testpass123"))
    machine.succeed(f"keyctl pipe {serial} | cmp - /run/fixture-pass; keyctl revoke {serial}")
    serial = export_password(measured=True)
    machine.succeed(f"keyctl pipe {serial} | cmp - /run/fixture-pass; keyctl revoke {serial}; cryptsetup close already-open-proof")
    machine.succeed('"$(cat /etc/test-cryptsetup)" attach already-open-proof /dev/vdb /run/fixture-pass x-nixos.opportunistic-passphrase </dev/null 2>&1')
    no_key()
    machine.succeed('"$(cat /etc/test-cryptsetup)" attach already-open-proof /dev/vdb - x-nixos.opportunistic-passphrase </dev/null 2>&1')
    no_key()

with subtest("LUKS1 and LUKS2 unlock across SD USB virtio and partitioned NVMe without mounting"):
    format_luks(SD, argon=True)
    format_luks(USB)
    format_luks(INTERNAL, kind="luks1")
    format_luks(NVME)
    export_password()
    worker()
    for device in DEVICES:
        assert_open(device)
    machine.fail("findmnt /run/media/volumeuser/OPTIONAL")
    close_devices()
    worker()
    for device in DEVICES:
        assert_closed(device)

with subtest("wrong volume passwords never borrow a rejected root candidate"):
    machine.succeed(f"cryptsetup luksChangeKey --key-file /run/fixture-pass {USB} /run/fixture-other", timeout=90)
    export_password(("differentpass", "testpass123"))
    worker()
    assert_closed(USB)
    for device in (SD, INTERNAL, NVME):
        assert_open(device)
    machine.succeed(f"cryptsetup open --key-file /run/fixture-other {USB} manual-volume; cryptsetup close manual-volume")
    close_devices()
    machine.succeed(f"cryptsetup luksChangeKey --key-file /run/fixture-other {USB} /run/fixture-pass", timeout=90)

with subtest("inherited ignored SIGCHLD cannot retain completed worker PIDs"):
    export_password()
    began = time.monotonic()
    machine.succeed("bash -c 'trap \"\" CHLD; exec nixos-opportunistic-unlock testroot'", timeout=10)
    assert time.monotonic() - began < 8, "completed workers retained stale process slots"
    no_key()
    for device in DEVICES:
        assert_open(device)
    close_devices()

with subtest("expired malformed or already-consumed handoff performs no unlock"):
    serial = export_password()
    machine.succeed(f"keyctl timeout {serial} 1")
    time.sleep(2)
    worker()
    for device in DEVICES:
        assert_closed(device)
    serial = export_password()
    machine.succeed(f"keyctl setperm {serial} 0x3f3f0000")
    worker()
    for device in DEVICES:
        assert_closed(device)

with subtest("an absent device is optional and later arrival never retries"):
    machine.succeed("basename $(realpath /sys/class/mmc_host/mmc0/device) > /run/sd-controller")
    machine.succeed("cat /run/sd-controller > /sys/bus/pci/drivers/sdhci-pci/unbind")
    machine.wait_until_fails(f"test -e {SD}")
    export_password()
    worker()
    machine.succeed("cat /run/sd-controller > /sys/bus/pci/drivers/sdhci-pci/bind")
    machine.wait_until_succeeds(f"test -b {SD}")
    time.sleep(1)
    assert_closed(SD)
    no_key()
    close_devices()

with subtest("duplicate LUKS UUIDs are distinct physical candidates"):
    machine.succeed(f"dd if={INTERNAL} of={USB} bs=4M count=64 conv=fsync status=none")
    machine.succeed(f"test $(cryptsetup luksUUID {INTERNAL}) = $(cryptsetup luksUUID {USB})")
    export_password()
    worker()
    assert assert_open(INTERNAL) != assert_open(USB)
    close_devices()

with subtest("malformed and unsupported headers are skipped while healthy volumes continue"):
    format_luks(USB, kind="luks1", cipher="aes-cbc-plain64")
    machine.succeed(f"wipefs -a {INTERNAL}; printf 'LUKS\\272\\276\\000\\002' | dd of={INTERNAL} conv=notrunc status=none")
    export_password()
    worker()
    assert_closed(USB)
    assert_closed(INTERNAL)
    assert_open(SD)
    assert_open(NVME)
    close_devices()
    format_luks(INTERNAL, kind="luks1")
    format_luks(USB)

with subtest("a slow first device cannot starve a later healthy volume or the session"):
    began = time.monotonic()
    start_stopped_candidate()
    machine.wait_until_succeeds(f"lsblk -nrpo TYPE {INTERNAL} | grep -x crypt", timeout=6)
    machine.fail("grep -aF testpass123 /proc/$(cat /run/stopped-worker)/cmdline /proc/$(cat /run/stopped-worker)/environ")
    machine.succeed(f"test $(systemctl show {SERVICE} -p MemorySwapMax --value) = 0; test $(systemctl show {SERVICE} -p LimitCORE --value) = 0")
    machine.fail(f"journalctl -b -u {SERVICE} | grep -F testpass123")
    wait_worker()
    assert time.monotonic() - began < 15, "per-device timeout exhausted the whole inventory budget"
    machine.fail("kill -0 $(cat /run/stopped-worker)")
    assert_closed(SD)
    assert_open(INTERNAL)
    close_devices()

with subtest("device replacement invalidates the captured inventory identity"):
    start_stopped_candidate()
    machine.succeed("cat /sys/class/block/mmcblk0/diskseq > /run/old-diskseq; cat /run/sd-controller > /sys/bus/pci/drivers/sdhci-pci/unbind; cat /run/sd-controller > /sys/bus/pci/drivers/sdhci-pci/bind")
    machine.wait_until_succeeds(f"test -b {SD}")
    machine.succeed("test $(cat /run/old-diskseq) != $(cat /sys/class/block/mmcblk0/diskseq); kill -CONT $(cat /run/stopped-worker)")
    wait_worker()
    assert_closed(SD)
    assert_open(INTERNAL)
    close_devices()

with subtest("device read failures stay local to that candidate"):
    machine.succeed(f"blockdev --flushbufs {SD}; echo 3 > /proc/sys/vm/drop_caches")
    cast(Any, machine.qmp_client).send("blockdev-add", {"driver": "blkdebug", "node-name": "sd-errors", "image": "sdfile", "inject-error": [{"event": "read_aio", "errno": 5, "immediately": True}]})
    cast(Any, machine.qmp_client).send("blockdev-reopen", {"options": [{"driver": "raw", "node-name": "sdcard", "file": "sd-errors"}]})
    export_password()
    worker()
    assert_closed(SD)
    assert_open(INTERNAL)
    cast(Any, machine.qmp_client).send("blockdev-reopen", {"options": [{"driver": "raw", "node-name": "sdcard", "file": "sdfile"}]})
    cast(Any, machine.qmp_client).send("blockdev-del", {"node-name": "sd-errors"})
    close_devices()

with subtest("unknown root identity consumes the key and fails closed"):
    export_password()
    machine.succeed("nixos-opportunistic-unlock nonexistent-root")
    no_key()
    for device in DEVICES:
        assert_closed(device)

with subtest("rejected then successful console password unlocks attached volumes on reboot"):
    machine.shutdown()
    boot(wrong_first=True)
    wait_worker()
    for device in DEVICES:
        assert_open(device)
    machine.succeed("systemctl is-active multi-user.target")
    close_devices()
    machine.succeed("umask 077; printf testpass123 > /run/fixture-pass")

with subtest("TPM-only activation and root boot export no passphrase"):
    machine.wait_until_succeeds("test -e /dev/tpmrm0")
    for mapping in mappings("/dev/vdb"):
        machine.succeed("cryptsetup close " + shlex.quote(mapping))
    machine.succeed("systemd-cryptenroll --unlock-key-file=/run/fixture-pass --tpm2-device=auto --tpm2-pcrs=7 /dev/vdb", timeout=60)
    machine.succeed("if cryptsetup status already-open-proof >/dev/null; then cryptsetup close already-open-proof; fi")
    machine.succeed('"$(cat /etc/test-cryptsetup)" attach already-open-proof /dev/vdb - tpm2-device=auto,x-nixos.opportunistic-passphrase </dev/null 2>&1')
    no_key()
    worker()
    for device in DEVICES:
        assert_closed(device)
    machine.succeed("systemd-cryptenroll --unlock-key-file=/run/fixture-pass --tpm2-device=auto --tpm2-pcrs=7 /dev/vda", timeout=60)
    machine.shutdown()
    machine.start()
    machine.wait_for_unit("multi-user.target", timeout=60)
    machine.succeed("findmnt -n -o SOURCE / | grep /dev/mapper/testroot")
    wait_worker()
    for device in DEVICES:
        assert_closed(device)
