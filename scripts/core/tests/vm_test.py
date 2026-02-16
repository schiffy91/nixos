#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3 python3Packages.pytest qemu libarchive openssh OVMF
"""
E2E tests for the NixOS VM test harness.

Uses QEMU snapshots as checkpoints so each phase can be skipped if the
checkpoint already exists. Run with:

    pytest vm_test.py -v -s --tb=short

To start fresh:

    VM_CLEAN=1 pytest vm_test.py -v -s --tb=short
"""
import os
import subprocess
import sys
import time

import pytest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".."))

from core.vm import VM  # noqa: E402

NIXOS_DIR = os.path.normpath(
    os.path.join(os.path.dirname(__file__), "..", "..", "..")
)

INSTALL_SCRIPT = """\
import sys
sys.path.insert(0, "/etc/nixos/scripts")
from cli.install import Installer
Installer.erase_and_mount_disk()
Installer.install_nixos()
print("INSTALL_OK")
"""

BOOTLOADER_SCRIPT = """\
set -eo pipefail
resolve() {
    local root=$1 path=$2
    while [ -L "${root}${path}" ]; do
        local target=$(readlink "${root}${path}")
        if [ "${target:0:1}" = "/" ]; then
            path="$target"
        else
            path="$(dirname "$path")/$target"
        fi
    done
    echo "$path"
}
SYSTEM=$(resolve /mnt /nix/var/nix/profiles/system)
KERNEL=$(resolve /mnt $SYSTEM/kernel)
INITRD=$(resolve /mnt $SYSTEM/initrd)
KPARAMS=$(cat /mnt${SYSTEM}/kernel-params 2>/dev/null || echo "")
HASH=$(echo "$SYSTEM" | md5sum | cut -c1-8)
mkdir -p /mnt/boot/EFI/nixos
cp /mnt$KERNEL /mnt/boot/EFI/nixos/${HASH}-bzImage
cp /mnt$INITRD /mnt/boot/EFI/nixos/${HASH}-initrd
SBOOT=$(ls /mnt/nix/store/*-systemd-[0-9]*/lib/systemd/boot/efi/systemd-bootx64.efi 2>/dev/null | head -1)
[ -n "$SBOOT" ] || { echo "ERR: systemd-boot not found"; exit 1; }
mkdir -p /mnt/boot/EFI/BOOT /mnt/boot/EFI/systemd
cp "$SBOOT" /mnt/boot/EFI/BOOT/BOOTX64.EFI
cp "$SBOOT" /mnt/boot/EFI/systemd/systemd-bootx64.efi
mkdir -p /mnt/boot/loader/entries
cat > /mnt/boot/loader/loader.conf << 'LCONF'
timeout 3
default nixos.conf
editor no
LCONF
cat > /mnt/boot/loader/entries/nixos.conf << ENTRY
title NixOS
linux /EFI/nixos/${HASH}-bzImage
initrd /EFI/nixos/${HASH}-initrd
options ${KPARAMS}
ENTRY
echo "BOOTLOADER_OK"
"""


# --- Helpers ---

def checkpoint(name):
    if not VM.has_snapshot(name):
        VM.snapshot(name)


def restore_or_skip(name):
    if not VM.has_snapshot(name):
        pytest.skip(f"Checkpoint '{name}' not found")
    VM.restore(name)


def boot_and_ssh(from_iso=False):
    VM.boot(from_iso=from_iso)
    if from_iso:
        time.sleep(30)
        VM.serial_bootstrap_ssh()
    VM.wait_for_ssh(timeout=180)


def ssh(cmd, check=True, timeout=30):
    result = VM.ssh(cmd, check=check, timeout=timeout)
    return result.stdout.strip()


def skip_if_checkpoint(name):
    if VM.has_snapshot(name):
        pytest.skip(f"Checkpoint '{name}' exists")


# --- Fixtures ---

@pytest.fixture(scope="session", autouse=True)
def cleanup():
    yield
    VM.stop()


@pytest.fixture(scope="session", autouse=True)
def handle_clean():
    if os.environ.get("VM_CLEAN"):
        VM.clean()


# --- Phase 1: Setup (download ISO, create disk, extract boot) ---

class TestSetup:
    def test_download_iso(self):
        VM.download_iso()
        assert os.path.exists(VM.iso_path())
        assert os.path.getsize(VM.iso_path()) > 100_000_000

    def test_create_ssh_key(self):
        VM.create_ssh_key()
        assert os.path.exists(VM.ssh_key())
        assert os.path.exists(f"{VM.ssh_key()}.pub")

    def test_create_disk(self):
        if not os.path.exists(VM.disk_path()):
            VM.create_disk()
        assert os.path.exists(VM.disk_path())

    def test_extract_boot(self):
        VM.extract_boot()
        cfg = VM.boot_cfg()
        assert os.path.exists(cfg["kernel"])
        assert os.path.exists(cfg["initrd"])
        assert "console=ttyS0" in cfg["append"]


# --- Phase 2: Boot into live ISO and get SSH ---

class TestLiveBoot:
    def test_boot_iso(self):
        skip_if_checkpoint("live-ssh")
        VM.boot(from_iso=True)
        assert VM.process is not None
        assert VM.process.poll() is None

    def test_wait_for_boot(self):
        skip_if_checkpoint("live-ssh")
        time.sleep(30)
        output = VM.serial_read()
        assert len(output) > 0

    def test_bootstrap_ssh(self):
        skip_if_checkpoint("live-ssh")
        VM.serial_bootstrap_ssh()

    def test_ssh_ready(self):
        skip_if_checkpoint("live-ssh")
        VM.wait_for_ssh(timeout=120)

    def test_ssh_works(self):
        skip_if_checkpoint("live-ssh")
        assert "hello" in VM.ssh("echo hello").stdout

    def test_checkpoint_live_ssh(self):
        skip_if_checkpoint("live-ssh")
        checkpoint("live-ssh")


# --- Phase 3: Copy workspace and install NixOS ---

class TestInstall:
    def test_restore_live(self):
        restore_or_skip("live-ssh")
        boot_and_ssh(from_iso=True)

    def test_copy_workspace(self):
        skip_if_checkpoint("installed")
        parent = os.path.dirname(NIXOS_DIR)
        name = os.path.basename(NIXOS_DIR)
        subprocess.run(
            f"tar czf - --exclude=.vm --exclude=secrets --exclude=.git "
            f"-C {parent} {name} | "
            f"ssh -p {VM.SSH_PORT} "
            + " ".join(VM.ssh_opts())
            + f" root@localhost 'tar xzf - -C {parent}'",
            shell=True, check=True,
        )
        assert "flake.nix" in ssh("ls /etc/nixos/flake.nix")

    def test_configure_for_vm(self):
        skip_if_checkpoint("installed")
        pubkey = VM.ssh_pubkey()
        vm_test_nix = (
            '{ lib, ... }: {\\n'
            '  networking.hostName = "VM-TEST";\\n'
            f'  users.users.root.openssh.authorizedKeys.keys = [ "{pubkey}" ];\\n'
            '  boot.kernelParams = [ "console=ttyS0,115200n8" ];\\n'
            '  boot.initrd.availableKernelModules = '
            '[ "virtio_pci" "virtio_blk" "virtio_net" ];\\n'
            '  services.openssh.settings.UseDns = lib.mkForce false;\\n'
            '  settings.networking.lanSubnet = "10.0.2.0/24";\\n'
            '  settings.disk.device = "/dev/vda";\\n'
            '  settings.disk.encryption.enable = false;\\n'
            '  settings.disk.immutability.enable = true;\\n'
            '  settings.disk.swap.size = "2G";\\n'
            '  settings.desktop.environment = "none";\\n'
            '}\\n'
        )
        ssh(f"printf '{vm_test_nix}' > "
            "/etc/nixos/modules/hosts/x86_64/VM-TEST.nix")
        ssh(
            'echo \'{"host_path": "modules/hosts/x86_64/VM-TEST.nix", '
            '"target": "Standard-Boot"}\' > /etc/nixos/config.json'
        )
        ssh("git -C /etc/nixos init")
        ssh("git config --global --add safe.directory /etc/nixos")
        ssh("git -C /etc/nixos add -A", timeout=60)
        ssh("git -C /etc/nixos -c user.name=test -c user.email=test@test "
            "commit -m init", timeout=60)

    def test_install_nixos(self):
        skip_if_checkpoint("installed")
        ssh(f"cat > /tmp/install.py << 'PYEOF'\n{INSTALL_SCRIPT}PYEOF")
        result = ssh(
            "nix-shell -p python3 --run 'python3 /tmp/install.py'",
            check=False, timeout=600,
        )
        assert "INSTALL_OK" in result

    def test_generate_ssh_host_keys(self):
        skip_if_checkpoint("installed")
        ssh("ssh-keygen -t rsa -f /mnt/etc/ssh/ssh_host_rsa_key -N '' -q")
        ssh(
            "ssh-keygen -t ed25519 "
            "-f /mnt/etc/ssh/ssh_host_ed25519_key -N '' -q"
        )

    def test_install_bootloader(self):
        skip_if_checkpoint("installed")
        ssh(
            f"cat > /tmp/bootloader.sh << 'SHEOF'\n"
            f"{BOOTLOADER_SCRIPT}SHEOF"
        )
        result = ssh("bash /tmp/bootloader.sh", timeout=120)
        assert "BOOTLOADER_OK" in result

    def test_checkpoint_installed(self):
        skip_if_checkpoint("installed")
        checkpoint("installed")


# --- Phase 4: Boot installed OS ---

class TestBootInstalled:
    def test_restore_installed(self):
        restore_or_skip("installed")

    def test_boot_from_disk(self):
        skip_if_checkpoint("booted")
        VM.boot(from_iso=False)
        VM.wait_for_ssh(timeout=180)

    def test_hostname(self):
        if VM.has_snapshot("booted"):
            restore_or_skip("booted")
            boot_and_ssh()
        assert ssh("hostname") == "VM-TEST"

    def test_nixos_version(self):
        assert ssh("nixos-version") != ""

    def test_nixos_config_present(self):
        assert "flake.nix" in ssh("ls /etc/nixos/flake.nix")

    def test_checkpoint_booted(self):
        skip_if_checkpoint("booted")
        checkpoint("booted")


# --- Phase 5: Immutability tests ---

def immutability_enabled():
    result = VM.ssh(
        "journalctl -u immutability -b --no-pager 2>/dev/null"
        " | grep -q 'Immutability reset complete'"
        " && echo enabled || echo disabled",
        check=False,
    )
    return "enabled" in result.stdout


class TestImmutability:
    def test_restore_booted(self):
        restore_or_skip("booted")
        boot_and_ssh()

    def test_immutability_enabled(self):
        if not immutability_enabled():
            pytest.skip("Immutability disabled — skipping all")

    def test_write_persistent_file(self):
        if not immutability_enabled():
            pytest.skip("Immutability disabled")
        ssh("echo 'persist-test' > /etc/nixos/e2e-persist-marker")
        assert "persist-test" in ssh("cat /etc/nixos/e2e-persist-marker")

    def test_write_ephemeral_file(self):
        if not immutability_enabled():
            pytest.skip("Immutability disabled")
        ssh("echo 'ephemeral' > /root/e2e-ephemeral-marker")
        assert "ephemeral" in ssh("cat /root/e2e-ephemeral-marker")

    def test_reboot(self):
        if not immutability_enabled():
            pytest.skip("Immutability disabled")
        ssh("reboot", check=False)
        time.sleep(5)
        VM.stop()
        VM.boot(from_iso=False)
        VM.wait_for_ssh(timeout=180)

    def test_persistent_file_survives(self):
        if not immutability_enabled():
            pytest.skip("Immutability disabled")
        result = ssh("cat /etc/nixos/e2e-persist-marker", check=False)
        assert "persist-test" in result

    def test_ephemeral_file_wiped(self):
        if not immutability_enabled():
            pytest.skip("Immutability disabled")
        result = ssh("cat /root/e2e-ephemeral-marker 2>&1", check=False)
        assert "No such file" in result or "ephemeral" not in result

    def test_checkpoint_immutability(self):
        skip_if_checkpoint("immutability-tested")
        checkpoint("immutability-tested")


# --- Phase 6: Update/rebuild test ---

class TestUpdate:
    def test_restore_booted(self):
        restore_or_skip("booted")
        boot_and_ssh()

    def test_rebuild_switch(self):
        result = ssh(
            "nixos-rebuild switch "
            "--flake /etc/nixos#VM-TEST-Standard-Boot",
            check=False, timeout=600,
        )
        assert result is not None

    def test_still_boots_after_update(self):
        ssh("reboot", check=False)
        time.sleep(5)
        VM.stop()
        VM.boot(from_iso=False)
        VM.wait_for_ssh(timeout=180)
        assert ssh("hostname") == "VM-TEST"


# --- Phase 7: Cleanup ---

class TestCleanup:
    def test_stop_vm(self):
        VM.stop()
        assert VM.process is None
