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
import shlex
import subprocess
import sys
import time

import pytest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", ".."))

from core.vm import VM, ARCH  # noqa: E402

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

_EFI_ARCH = "aa64" if ARCH == "aarch64" else "x64"
_KERNEL_NAME = "Image" if ARCH == "aarch64" else "bzImage"

BOOTLOADER_SCRIPT = f"""\
set -eo pipefail
resolve() {{
    local root=$1 path=$2
    while [ -L "${{root}}${{path}}" ]; do
        local target=$(readlink "${{root}}${{path}}")
        if [ "${{target:0:1}}" = "/" ]; then
            path="$target"
        else
            path="$(dirname "$path")/$target"
        fi
    done
    echo "$path"
}}
SYSTEM=$(resolve /mnt /nix/var/nix/profiles/system)
KERNEL=$(resolve /mnt $SYSTEM/kernel)
INITRD=$(resolve /mnt $SYSTEM/initrd)
KPARAMS=$(cat /mnt${{SYSTEM}}/kernel-params 2>/dev/null || echo "")
HASH=$(echo "$SYSTEM" | md5sum | cut -c1-8)
mkdir -p /mnt/boot/EFI/nixos
cp /mnt$KERNEL /mnt/boot/EFI/nixos/${{HASH}}-{_KERNEL_NAME}
cp /mnt$INITRD /mnt/boot/EFI/nixos/${{HASH}}-initrd
SBOOT=$(ls /mnt/nix/store/*-systemd-[0-9]*/lib/systemd/boot/efi/systemd-boot{_EFI_ARCH}.efi 2>/dev/null | head -1)
[ -n "$SBOOT" ] || {{ echo "ERR: systemd-boot not found"; exit 1; }}
mkdir -p /mnt/boot/EFI/BOOT /mnt/boot/EFI/systemd
cp "$SBOOT" /mnt/boot/EFI/BOOT/BOOT{_EFI_ARCH.upper()}.EFI
cp "$SBOOT" /mnt/boot/EFI/systemd/systemd-boot{_EFI_ARCH}.efi
mkdir -p /mnt/boot/loader/entries
cat > /mnt/boot/loader/loader.conf << 'LCONF'
timeout 3
default nixos.conf
editor no
LCONF
cat > /mnt/boot/loader/entries/nixos.conf << ENTRY
title NixOS
linux /EFI/nixos/${{HASH}}-{_KERNEL_NAME}
initrd /EFI/nixos/${{HASH}}-initrd
options ${{KPARAMS}}
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


def reboot_vm():
    ssh("reboot", check=False)
    time.sleep(5)
    VM.stop()
    VM.boot(from_iso=False)
    VM.wait_for_ssh(timeout=180)


def change_mode_and_rebuild(mode):
    nix_path = f"/etc/nixos/modules/hosts/{ARCH}/VM-TEST.nix"
    has_mode = ssh(
        f"grep -c 'immutability.mode' {nix_path} || true", check=False,
    )
    if has_mode.strip() == "0":
        ssh(
            f"sed -i '/immutability.enable/a\\"
            f"  settings.disk.immutability.mode = \"{mode}\";' {nix_path}"
        )
    else:
        ssh(
            f"sed -i 's|immutability.mode = \"[^\"]*\"|"
            f"immutability.mode = \"{mode}\"|' {nix_path}"
        )
    result = ssh(f"grep 'immutability.mode' {nix_path}")
    assert mode in result, f"Mode change failed: expected '{mode}' in '{result}'"
    ssh("git -C /etc/nixos add -A", timeout=60)
    ssh(
        "git -C /etc/nixos -c user.name=test -c user.email=test@test "
        f"commit -m 'Switch to {mode} mode' --allow-empty",
        timeout=60,
    )
    r = VM.ssh(
        "nixos-rebuild switch --flake /etc/nixos#VM-TEST-Standard-Boot",
        check=False, timeout=600,
    )
    output = r.stdout + r.stderr
    assert "activating the configuration" in output, (
        f"nixos-rebuild failed (rc={r.returncode}):\n"
        + output[-2000:]
    )
    reboot_vm()


def write_marker(path, content):
    parent = os.path.dirname(path)
    ssh(f"mkdir -p {parent}")
    ssh(f"echo '{content}' > {path}")


def check_marker(path, content, should_exist=True):
    result = ssh(f"cat {path} 2>&1", check=False)
    if should_exist:
        assert "No such file" not in result, f"Expected {path} to exist"
        assert content in result, (
            f"Expected '{content}' in {path}, got '{result}'"
        )
    else:
        assert "No such file" in result or content not in result, (
            f"Expected {path} to not exist or not contain '{content}'"
        )


def snapshot_has_file(subvol, label, rel_path):
    result = ssh(
        f"test -f /.snapshots/{subvol}/{label}/{rel_path}"
        " && echo yes || echo no",
        check=False,
    )
    return "yes" in result


def snapshot_file_content(subvol, label, rel_path):
    return ssh(
        f"cat /.snapshots/{subvol}/{label}/{rel_path} 2>/dev/null",
        check=False,
    )


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
        if ARCH == "x86_64":
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
        ssh_opts = " ".join(shlex.quote(o) for o in VM.ssh_opts())
        subprocess.run(
            f"tar czf - --exclude=.vm --exclude=secrets --exclude=.git "
            f"--exclude='._*' --no-mac-metadata "
            f"-C {shlex.quote(parent)} {shlex.quote(name)} | "
            f"ssh -p {VM.SSH_PORT} {ssh_opts}"
            f" root@localhost 'mkdir -p /etc && tar xzf - -C /etc'",
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
            '  settings.disk.immutability.mode = "reset";\\n'
            '  settings.disk.swap.size = "2G";\\n'
            '  settings.desktop.environment = "none";\\n'
            '}\\n'
        )
        ssh(f"mkdir -p /etc/nixos/modules/hosts/{ARCH}")
        ssh(f"printf '{vm_test_nix}' > "
            f"/etc/nixos/modules/hosts/{ARCH}/VM-TEST.nix")
        ssh(
            f'echo \'{{"host_path": "modules/hosts/{ARCH}/VM-TEST.nix", '
            f'"target": "Standard-Boot"}}\' > /etc/nixos/config.json'
        )
        ssh("git -C /etc/nixos init")
        ssh("git config --global --add safe.directory /etc/nixos")
        ssh("git -C /etc/nixos add -A", timeout=60)
        ssh("git -C /etc/nixos -c user.name=test -c user.email=test@test "
            "commit -m init", timeout=60)

    def test_install_nixos(self):
        skip_if_checkpoint("installed")
        ssh(f"cat > /tmp/install.py << 'PYEOF'\n{INSTALL_SCRIPT}PYEOF")
        r = VM.ssh(
            "nix-shell -p python3 --run 'python3 /tmp/install.py'",
            check=False, timeout=600,
        )
        output = r.stdout + r.stderr
        assert "INSTALL_OK" in output, (
            f"Install failed (rc={r.returncode}):\n"
            + output[-2000:]
        )

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
        result = ssh("test -f /mnt/etc/nixos/flake.nix && echo ok", check=False)
        assert "ok" in result, "Install incomplete — not checkpointing"
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


# --- Phase 5a: Immutability — reset mode ---


def immutability_log():
    return ssh(
        "journalctl -u immutability -b --no-pager 2>/dev/null",
        check=False,
    )


def immutability_completed():
    return "Immutability complete" in immutability_log()


class TestImmutabilityReset:
    """Reset mode: ephemeral paths wiped, persistent paths kept."""

    def test_restore_booted(self):
        restore_or_skip("booted")
        boot_and_ssh()

    def test_immutability_ran(self):
        skip_if_checkpoint("reset-tested")
        assert immutability_completed(), (
            "Immutability service did not complete:\n" + immutability_log()
        )

    def test_snapshots_exist(self):
        skip_if_checkpoint("reset-tested")
        for subvol in ("@root", "@home"):
            result = ssh(
                f"test -d /.snapshots/{subvol}/CLEAN && echo yes || echo no",
                check=False,
            )
            assert "yes" in result, f"CLEAN snapshot missing for {subvol}"

    def test_write_persistent_root(self):
        skip_if_checkpoint("reset-tested")
        write_marker("/etc/nixos/e2e-persist-marker", "persist-root")
        check_marker("/etc/nixos/e2e-persist-marker", "persist-root")

    def test_write_persistent_home(self):
        skip_if_checkpoint("reset-tested")
        user = ssh("ls /home/ | head -1")
        write_marker(f"/home/{user}/.cache/e2e-home-marker", "persist-home")
        check_marker(f"/home/{user}/.cache/e2e-home-marker", "persist-home")

    def test_write_ephemeral_root(self):
        skip_if_checkpoint("reset-tested")
        write_marker("/root/e2e-ephemeral-marker", "ephemeral-root")
        check_marker("/root/e2e-ephemeral-marker", "ephemeral-root")

    def test_write_ephemeral_home(self):
        skip_if_checkpoint("reset-tested")
        user = ssh("ls /home/ | head -1")
        write_marker(f"/home/{user}/e2e-ephemeral-home", "ephemeral-home")
        check_marker(f"/home/{user}/e2e-ephemeral-home", "ephemeral-home")

    def test_reboot(self):
        skip_if_checkpoint("reset-tested")
        reboot_vm()

    def test_immutability_ran_after_reboot(self):
        skip_if_checkpoint("reset-tested")
        assert immutability_completed()

    def test_persistent_root_survives(self):
        skip_if_checkpoint("reset-tested")
        check_marker("/etc/nixos/e2e-persist-marker", "persist-root")

    def test_persistent_home_survives(self):
        skip_if_checkpoint("reset-tested")
        user = ssh("ls /home/ | head -1")
        check_marker(f"/home/{user}/.cache/e2e-home-marker", "persist-home")

    def test_ephemeral_root_wiped(self):
        skip_if_checkpoint("reset-tested")
        check_marker(
            "/root/e2e-ephemeral-marker", "ephemeral-root",
            should_exist=False,
        )

    def test_ephemeral_home_wiped(self):
        skip_if_checkpoint("reset-tested")
        user = ssh("ls /home/ | head -1")
        check_marker(
            f"/home/{user}/e2e-ephemeral-home", "ephemeral-home",
            should_exist=False,
        )

    def test_previous_has_ephemeral(self):
        skip_if_checkpoint("reset-tested")
        assert snapshot_has_file(
            "@root", "PREVIOUS", "root/e2e-ephemeral-marker"
        ), "PREVIOUS should contain ephemeral file from last boot"
        content = snapshot_file_content(
            "@root", "PREVIOUS", "root/e2e-ephemeral-marker"
        )
        assert "ephemeral-root" in content

    def test_checkpoint(self):
        skip_if_checkpoint("reset-tested")
        checkpoint("reset-tested")


# --- Phase 5b: Immutability — snapshot-only mode ---


class TestImmutabilitySnapshotOnly:
    """Snapshot-only: snapshots rotate but filesystem is NOT wiped."""

    def test_restore_booted(self):
        restore_or_skip("booted")
        boot_and_ssh()

    def test_switch_mode(self):
        skip_if_checkpoint("snapshot-only-rebuilt")
        change_mode_and_rebuild("snapshot-only")

    def test_checkpoint_rebuilt(self):
        skip_if_checkpoint("snapshot-only-rebuilt")
        checkpoint("snapshot-only-rebuilt")

    def test_restore_rebuilt(self):
        if not VM.has_snapshot("snapshot-only-rebuilt"):
            pytest.skip("Need snapshot-only-rebuilt")
        if VM.has_snapshot("snapshot-only-tested"):
            pytest.skip("Already tested")
        VM.restore("snapshot-only-rebuilt")
        boot_and_ssh()

    def test_immutability_ran(self):
        skip_if_checkpoint("snapshot-only-tested")
        assert immutability_completed()

    def test_write_markers(self):
        skip_if_checkpoint("snapshot-only-tested")
        user = ssh("ls /home/ | head -1")
        write_marker("/etc/nixos/e2e-snap-persist", "snap-persist")
        write_marker("/root/e2e-snap-ephemeral", "snap-ephemeral")
        write_marker(
            f"/home/{user}/.cache/e2e-snap-home-persist",
            "snap-home-persist",
        )
        write_marker(
            f"/home/{user}/e2e-snap-home-ephemeral",
            "snap-home-ephemeral",
        )

    def test_reboot(self):
        skip_if_checkpoint("snapshot-only-tested")
        reboot_vm()

    def test_immutability_ran_after_reboot(self):
        skip_if_checkpoint("snapshot-only-tested")
        assert immutability_completed()

    def test_persistent_root_survives(self):
        skip_if_checkpoint("snapshot-only-tested")
        check_marker("/etc/nixos/e2e-snap-persist", "snap-persist")

    def test_ephemeral_root_survives(self):
        """In snapshot-only, ephemeral files also survive."""
        skip_if_checkpoint("snapshot-only-tested")
        check_marker("/root/e2e-snap-ephemeral", "snap-ephemeral")

    def test_persistent_home_survives(self):
        skip_if_checkpoint("snapshot-only-tested")
        user = ssh("ls /home/ | head -1")
        check_marker(
            f"/home/{user}/.cache/e2e-snap-home-persist",
            "snap-home-persist",
        )

    def test_ephemeral_home_survives(self):
        skip_if_checkpoint("snapshot-only-tested")
        user = ssh("ls /home/ | head -1")
        check_marker(
            f"/home/{user}/e2e-snap-home-ephemeral",
            "snap-home-ephemeral",
        )

    def test_previous_captured(self):
        skip_if_checkpoint("snapshot-only-tested")
        assert snapshot_has_file(
            "@root", "PREVIOUS", "root/e2e-snap-ephemeral"
        ), "PREVIOUS should contain pre-reboot state"

    def test_checkpoint(self):
        skip_if_checkpoint("snapshot-only-tested")
        checkpoint("snapshot-only-tested")


# --- Phase 5c: Immutability — disabled mode ---


class TestImmutabilityDisabled:
    """Disabled mode: no immutability operations, everything survives."""

    def test_restore_booted(self):
        restore_or_skip("booted")
        boot_and_ssh()

    def test_switch_mode(self):
        skip_if_checkpoint("disabled-rebuilt")
        change_mode_and_rebuild("disabled")

    def test_checkpoint_rebuilt(self):
        skip_if_checkpoint("disabled-rebuilt")
        checkpoint("disabled-rebuilt")

    def test_restore_rebuilt(self):
        if not VM.has_snapshot("disabled-rebuilt"):
            pytest.skip("Need disabled-rebuilt")
        if VM.has_snapshot("disabled-tested"):
            pytest.skip("Already tested")
        VM.restore("disabled-rebuilt")
        boot_and_ssh()

    def test_service_reports_disabled(self):
        skip_if_checkpoint("disabled-tested")
        log = immutability_log()
        assert "disabled" in log.lower(), (
            f"Expected 'disabled' in immutability log:\n{log}"
        )

    def test_write_markers(self):
        skip_if_checkpoint("disabled-tested")
        user = ssh("ls /home/ | head -1")
        write_marker("/etc/nixos/e2e-disabled-persist", "disabled-persist")
        write_marker("/root/e2e-disabled-ephemeral", "disabled-ephemeral")
        write_marker(
            f"/home/{user}/e2e-disabled-home", "disabled-home",
        )

    def test_reboot(self):
        skip_if_checkpoint("disabled-tested")
        reboot_vm()

    def test_all_files_survive(self):
        skip_if_checkpoint("disabled-tested")
        user = ssh("ls /home/ | head -1")
        check_marker("/etc/nixos/e2e-disabled-persist", "disabled-persist")
        check_marker("/root/e2e-disabled-ephemeral", "disabled-ephemeral")
        check_marker(f"/home/{user}/e2e-disabled-home", "disabled-home")

    def test_still_reports_disabled(self):
        skip_if_checkpoint("disabled-tested")
        log = immutability_log()
        assert "disabled" in log.lower()

    def test_checkpoint(self):
        skip_if_checkpoint("disabled-tested")
        checkpoint("disabled-tested")


# --- Phase 5d: Immutability — restore-previous mode ---


class TestImmutabilityRestorePrevious:
    """
    Restore-previous: replaces live filesystem with PREVIOUS snapshot.

    Setup: build snapshot history via 3 boot cycles in reset mode.
    Each cycle plants a unique ephemeral marker so we can identify
    which snapshot gets restored.
    """

    def test_restore_booted(self):
        restore_or_skip("booted")
        boot_and_ssh()

    # --- Build snapshot history (3 boot cycles in reset mode) ---

    def test_cycle_1_write(self):
        skip_if_checkpoint("restore-history-ready")
        write_marker("/root/cycle-marker", "cycle-1")

    def test_cycle_1_reboot(self):
        skip_if_checkpoint("restore-history-ready")
        reboot_vm()
        assert immutability_completed()

    def test_cycle_1_verify(self):
        skip_if_checkpoint("restore-history-ready")
        check_marker("/root/cycle-marker", "cycle-1", should_exist=False)
        assert snapshot_has_file("@root", "PREVIOUS", "root/cycle-marker")
        assert "cycle-1" in snapshot_file_content(
            "@root", "PREVIOUS", "root/cycle-marker"
        )

    def test_cycle_2_write(self):
        skip_if_checkpoint("restore-history-ready")
        write_marker("/root/cycle-marker", "cycle-2")

    def test_cycle_2_reboot(self):
        skip_if_checkpoint("restore-history-ready")
        reboot_vm()
        assert immutability_completed()

    def test_cycle_2_verify(self):
        skip_if_checkpoint("restore-history-ready")
        pen = snapshot_file_content("@root", "PENULTIMATE", "root/cycle-marker")
        assert "cycle-1" in pen, f"PENULTIMATE should have cycle-1, got: {pen}"
        prev = snapshot_file_content("@root", "PREVIOUS", "root/cycle-marker")
        assert "cycle-2" in prev, f"PREVIOUS should have cycle-2, got: {prev}"

    def test_cycle_3_write(self):
        skip_if_checkpoint("restore-history-ready")
        write_marker("/root/cycle-marker", "cycle-3")

    def test_checkpoint_history(self):
        skip_if_checkpoint("restore-history-ready")
        checkpoint("restore-history-ready")

    # --- Switch to restore-previous and test ---

    def test_restore_history(self):
        if not VM.has_snapshot("restore-history-ready"):
            pytest.skip("Need restore-history-ready")
        if VM.has_snapshot("restore-previous-tested"):
            pytest.skip("Already tested")
        VM.restore("restore-history-ready")
        boot_and_ssh()

    def test_switch_mode(self):
        skip_if_checkpoint("restore-previous-rebuilt")
        change_mode_and_rebuild("restore-previous")

    def test_checkpoint_rebuilt(self):
        skip_if_checkpoint("restore-previous-rebuilt")
        checkpoint("restore-previous-rebuilt")

    def test_restore_rebuilt(self):
        if VM.has_snapshot("restore-previous-rebuilt") \
                and not VM.has_snapshot("restore-previous-tested"):
            VM.restore("restore-previous-rebuilt")
            boot_and_ssh()
        elif VM.has_snapshot("restore-previous-tested"):
            pytest.skip("Already tested")

    def test_restore_logged(self):
        skip_if_checkpoint("restore-previous-tested")
        log = immutability_log()
        assert "Restoring" in log or "PREVIOUS" in log, (
            f"Expected restore log:\n{log}"
        )

    def test_write_fresh_marker(self):
        skip_if_checkpoint("restore-previous-tested")
        write_marker("/root/restore-prev-test", "should-disappear")
        check_marker("/root/restore-prev-test", "should-disappear")

    def test_reboot_restores_previous(self):
        skip_if_checkpoint("restore-previous-tested")
        reboot_vm()
        check_marker(
            "/root/restore-prev-test", "should-disappear",
            should_exist=False,
        )

    def test_system_functional(self):
        skip_if_checkpoint("restore-previous-tested")
        assert ssh("hostname") == "VM-TEST"
        result = ssh("test -f /etc/nixos/flake.nix && echo ok", check=False)
        assert "ok" in result

    def test_checkpoint(self):
        skip_if_checkpoint("restore-previous-tested")
        checkpoint("restore-previous-tested")


# --- Phase 5e: Immutability — restore-penultimate mode ---


class TestImmutabilityRestorePenultimate:
    """
    Restore-penultimate: replaces live with PENULTIMATE snapshot.

    Reuses the snapshot history built by TestImmutabilityRestorePrevious.
    """

    def test_restore_history(self):
        if not VM.has_snapshot("restore-history-ready"):
            pytest.skip("Need restore-history-ready")
        if VM.has_snapshot("restore-penultimate-tested"):
            pytest.skip("Already tested")
        VM.restore("restore-history-ready")
        boot_and_ssh()

    def test_switch_mode(self):
        skip_if_checkpoint("restore-penultimate-rebuilt")
        change_mode_and_rebuild("restore-penultimate")

    def test_checkpoint_rebuilt(self):
        skip_if_checkpoint("restore-penultimate-rebuilt")
        checkpoint("restore-penultimate-rebuilt")

    def test_restore_rebuilt(self):
        if VM.has_snapshot("restore-penultimate-rebuilt") \
                and not VM.has_snapshot("restore-penultimate-tested"):
            VM.restore("restore-penultimate-rebuilt")
            boot_and_ssh()
        elif VM.has_snapshot("restore-penultimate-tested"):
            pytest.skip("Already tested")

    def test_restore_logged(self):
        skip_if_checkpoint("restore-penultimate-tested")
        log = immutability_log()
        assert "Restoring" in log or "PENULTIMATE" in log, (
            f"Expected restore log:\n{log}"
        )

    def test_write_fresh_marker(self):
        skip_if_checkpoint("restore-penultimate-tested")
        write_marker("/root/restore-pen-test", "should-disappear")
        check_marker("/root/restore-pen-test", "should-disappear")

    def test_reboot_restores_penultimate(self):
        skip_if_checkpoint("restore-penultimate-tested")
        reboot_vm()
        check_marker(
            "/root/restore-pen-test", "should-disappear",
            should_exist=False,
        )

    def test_system_functional(self):
        skip_if_checkpoint("restore-penultimate-tested")
        assert ssh("hostname") == "VM-TEST"
        result = ssh("test -f /etc/nixos/flake.nix && echo ok", check=False)
        assert "ok" in result

    def test_checkpoint(self):
        skip_if_checkpoint("restore-penultimate-tested")
        checkpoint("restore-penultimate-tested")


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
        reboot_vm()
        assert ssh("hostname") == "VM-TEST"


# --- Phase 7: Cleanup ---

class TestCleanup:
    def test_stop_vm(self):
        VM.stop()
        assert VM.process is None
