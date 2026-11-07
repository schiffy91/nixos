import json
import shlex
import time

TOKEN_TEST = "LD_LIBRARY_PATH=/run/current-system/systemd/lib/cryptsetup cryptsetup open --test-passphrase --token-only /dev/vdb"


def cold_boot(password=False):
    machine.shutdown()
    machine.start()
    if password:
        machine.wait_for_console_text(r"Starting password query on /dev/ttyS0\.", timeout=120)
        time.sleep(0.3)
        machine.send_console("testpass123\n")
    machine.wait_for_unit("multi-user.target", timeout=180)


def default_generation(number):
    machine.succeed(f"bootctl set-default $(basename /boot/EFI/Linux/nixos-generation-{number}-*.efi)")


def recovery_command(command, label):
    # The literal command echo cannot match the expanded result marker.
    machine.send_console(f"bash -eu -c {shlex.quote(command)}; printf '\\n{label}_%s\\n' \"$?\"\n")
    machine.wait_for_console_text(label + "_0", timeout=180)


def wait_recovery():
    machine.wait_for_console_text("nixos-recovery login: root", timeout=240)
    # The serial reader emits complete lines; the shell prompt has no newline.
    machine.send_console("\n")
    machine.wait_for_console_text("root@nixos-recovery", timeout=30)


def cpio_entry(name, content, mode):
    name_bytes = name.encode() + b"\0"
    fields = [1, mode, 0, 0, 1, 0, len(content), 0, 0, 0, 0, len(name_bytes), 0]
    header = ("070701" + "".join(f"{value:08x}" for value in fields)).encode()
    result = header + name_bytes
    result += b"\0" * (-len(result) % 4)
    result += content
    result += b"\0" * (-len(content) % 4)
    return result


def writable_store():
    machine.succeed("mkdir -p /var/tmp/nix-upper /var/tmp/nix-work; mount -t overlay overlay -o lowerdir=/nix/store,upperdir=/var/tmp/nix-upper,workdir=/var/tmp/nix-work /nix/store")
    machine.succeed("cp -L /etc/nixos/flake.nix /var/tmp/flake.nix; mv -fT /var/tmp/flake.nix /etc/nixos/flake.nix")


def password_change_refused(reason):
    account_before = machine.succeed("getent shadow admin")
    luks_before = machine.succeed("cryptsetup luksDump --dump-json-metadata /dev/vdb")
    refused = machine.fail("NIXOS_OLD_PASSWORD=testpass123 NIXOS_NEW_PASSWORD=unapplied456 nixosctl change-password --update-tpm2 2>&1")
    assert reason in refused, refused
    assert "Managed TPM enrollment is unavailable; no passwords changed" in refused, refused
    assert machine.succeed("getent shadow admin") == account_before
    assert machine.succeed("cryptsetup luksDump --dump-json-metadata /dev/vdb") == luks_before
    machine.succeed("printf testpass123 | cryptsetup open --test-passphrase --key-file=- /dev/vdb")
    machine.fail("printf unapplied456 | cryptsetup open --test-passphrase --key-file=- /dev/vdb")


machine.wait_for_unit("multi-user.target")
writable_store()
assert "Secure Boot: enabled" in machine.succeed("bootctl status")
base = machine.succeed("readlink -f /run/current-system").strip()
first = machine.succeed("readlink -f /run/current-system/specialisation/secure-root").strip()
updated = machine.succeed("readlink -f /run/current-system/specialisation/updated").strip()

with subtest("prepare actual encrypted root and preserve passphrase"):
    machine.succeed("printf testpass123 | cryptsetup luksFormat --batch-mode --type luks2 /dev/vdb -")
    machine.succeed("printf testpass123 | cryptsetup open --key-file=- /dev/vdb root")
    machine.succeed(("mkfs.btrfs -f" if reset else "mkfs.ext4") + " /dev/mapper/root; mkdir -p /mnt/encrypted; mount /dev/mapper/root /mnt/encrypted")
    if reset:
        machine.succeed("for name in @root @var @nix @snapshots; do btrfs subvolume create /mnt/encrypted/$name; done; umount /mnt/encrypted; mount -o subvol=@root /dev/mapper/root /mnt/encrypted; mkdir -p /mnt/encrypted/{var,nix,.snapshots}; mount -o subvol=@var /dev/mapper/root /mnt/encrypted/var; mount -o subvol=@nix /dev/mapper/root /mnt/encrypted/nix")
    machine.fail("nixosctl tpm2 enable")
    password_change_refused("Enable the managed TPM boot policy")
    # Activated settings cannot make an already-running old boot eligible.
    # Exercise the real CLI/helper before any account or LUKS mutation.
    machine.fail("test -e /etc/nixosctl/tpm-policy")
    machine.succeed(f"mkdir -p /etc/nixosctl; ln -s {shlex.quote(first)}/etc/nixosctl/tpm-policy /etc/nixosctl/tpm-policy; sed -i 's/managedPolicy.enable = false/managedPolicy.enable = true/' /etc/nixos/flake.nix")
    password_change_refused("Boot an eligible generation")
    machine.succeed("rm /etc/nixosctl/tpm-policy; sed -i 's/managedPolicy.enable = true/managedPolicy.enable = false/' /etc/nixos/flake.nix")
    machine.succeed(f"ln -s {shlex.quote(first)} /nix/var/nix/profiles/system-2-link")
    # Keep the old boot entry outside Lanzaboote's generation GC for negatives.
    machine.succeed("mkdir -p /boot/EFI/test; cp /boot/EFI/Linux/nixos-generation-1-*.efi /boot/EFI/test/")
    machine.succeed("sbsign --key /var/lib/lanzaboote-test-fixture/keys/db/db.key --cert /var/lib/lanzaboote-test-fixture/keys/db/db.pem --output /boot/EFI/test/recovery.efi /etc/tpm-test/recovery.efi", timeout=120)
    machine.succeed("mkdir -p /boot/loader/entries; printf 'title Actual NixOS Recovery\nefi /EFI/test/recovery.efi\n' > /boot/loader/entries/recovery.conf")
    recovery_system = machine.succeed("cat /etc/tpm-test/recovery-system").strip()
    machine.fail(f"test -e {shlex.quote(recovery_system)}/tpm-policy-version")
    machine.succeed("rm /nix/var/nix/profiles/system-1-link")
    machine.succeed(f"{shlex.quote(first)}/bin/switch-to-configuration boot")
    machine.succeed("tar --one-file-system --exclude=./mnt --exclude=./proc --exclude=./sys --exclude=./dev --exclude=./run -C / -cf - . | tar -C /mnt/encrypted -xf -", timeout=120)
    machine.succeed("mkdir -p /mnt/encrypted/{mnt,proc,sys,dev,run,nix/store,boot}; sync")
    if reset:
        machine.succeed("umount /mnt/encrypted/nix; umount /mnt/encrypted/var; umount /mnt/encrypted; mount /dev/mapper/root /mnt/encrypted; mkdir -p /mnt/encrypted/@snapshots/@root; btrfs subvolume snapshot -r /mnt/encrypted/@root /mnt/encrypted/@snapshots/@root/CLEAN")
    machine.succeed("umount /mnt/encrypted; cryptsetup close root")
    machine.succeed("bootctl set-default $(basename /boot/EFI/Linux/nixos-generation-2-*.efi)")

with subtest("first eligible firmware boot requires the passphrase"):
    machine.shutdown()
    machine.start()
    machine.wait_for_console_text(r"Starting password query on /dev/ttyS0\.", timeout=120)
    time.sleep(0.3)
    machine.send_console("testpass123\n")
    machine.wait_for_unit("multi-user.target")
    writable_store()
    assert machine.succeed("findmnt -n -o SOURCE /").strip().split("[", 1)[0] == "/dev/mapper/root"
    assert machine.succeed("readlink -f /run/booted-system").strip() == first
    machine.wait_for_unit("systemd-pcrlock-make-policy.service")
    status = json.loads(machine.succeed("nixos-tpm-policy status"))
    assert set(status["pcrs"]) == {4, 7, 13}, status

with subtest("real nixosctl enrollment retains a passphrase and no PIN"):
    machine.succeed("PASSWORD=testpass123 nixosctl tpm2 enable", timeout=120)
    data = json.loads(machine.succeed("cryptsetup luksDump --dump-json-metadata /dev/vdb"))
    tokens = [token for token in data["tokens"].values() if token["type"] == "systemd-tpm2"]
    assert len(tokens) == 1 and tokens[0]["tpm2_pcrlock"]
    assert tokens[0]["tpm2-pcrs"] == [13]
    assert not tokens[0].get("tpm2-pin")
    machine.succeed("printf testpass123 | cryptsetup open --test-passphrase --key-file=- /dev/vdb")

with subtest("identical policy update does not rewrite NV or exported metadata"):
    machine.succeed("cp /var/lib/systemd/pcrlock.json /var/tmp/unchanged-policy.json; cp /boot/loader/credentials/pcrlock.nixosctl.cred /var/tmp/unchanged-policy.cred")
    output = machine.succeed("nixos-tpm-policy update 2>&1")
    assert "Prediction is identical to current policy, skipping update." in output
    assert "Updated NV index" not in output
    machine.succeed("cmp /var/tmp/unchanged-policy.json /var/lib/systemd/pcrlock.json; cmp /var/tmp/unchanged-policy.cred /boot/loader/credentials/pcrlock.nixosctl.cred")

with subtest("cold firmware boot automatically unlocks the real root"):
    machine.shutdown()
    machine.start()
    machine.wait_for_unit("multi-user.target", timeout=180)
    assert machine.succeed("findmnt -n -o SOURCE /").strip().split("[", 1)[0] == "/dev/mapper/root"
    machine.succeed("LD_LIBRARY_PATH=/run/current-system/systemd/lib/cryptsetup cryptsetup open --test-passphrase --token-only /dev/vdb")
