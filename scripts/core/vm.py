#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3 qemu libarchive openssh OVMF
import argparse
import json
import os
import platform
import re
import shutil
import subprocess
import threading
import time

MACOS = platform.system() == "Darwin"
ARCH = "aarch64" if platform.machine() == "arm64" else "x86_64"


class VM:
    DIR = os.path.normpath(os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "..", ".vm"
    ))
    ISO_URL = (
        "https://channels.nixos.org/nixos-unstable/"
        f"latest-nixos-minimal-{ARCH}-linux.iso"
    )
    SSH_PORT = 2222
    process: "subprocess.Popen[bytes] | None" = None
    output: list = []
    reader: "threading.Thread | None" = None

    # Paths
    @classmethod
    def iso_path(cls):
        return os.path.join(cls.DIR, "iso", "nixos-minimal.iso")

    @classmethod
    def disk_path(cls):
        return os.path.join(cls.DIR, "disk.qcow2")

    @classmethod
    def ovmf_path(cls):
        return os.path.join(cls.DIR, "OVMF_VARS.fd")

    @classmethod
    def ovmf_code(cls):
        if MACOS:
            brew_path = f"/opt/homebrew/share/qemu/edk2-{ARCH}-code.fd"
            if os.path.exists(brew_path):
                return brew_path
        result = subprocess.run(
            ["find", "/nix/store", "-maxdepth", "3",
             "-name", "OVMF_CODE.fd", "-path", "*OVMF*fd*"],
            capture_output=True, text=True, check=False
        )
        paths = result.stdout.strip().split("\n")
        if paths and paths[0]:
            return paths[0]
        raise FileNotFoundError("UEFI firmware not found")

    @classmethod
    def boot_dir(cls):
        return os.path.join(cls.DIR, "boot")

    @classmethod
    def ssh_key(cls):
        return os.path.join(cls.DIR, "id_ed25519")

    @classmethod
    def ssh_pubkey(cls):
        with open(f"{cls.ssh_key()}.pub", encoding="utf-8") as f:
            return f.read().strip()

    # Setup
    @classmethod
    def download_iso(cls):
        if os.path.exists(cls.iso_path()):
            return
        os.makedirs(os.path.dirname(cls.iso_path()), exist_ok=True)
        subprocess.run(
            ["curl", "-L", "-o", f"{cls.iso_path()}.tmp", cls.ISO_URL],
            check=True
        )
        os.rename(f"{cls.iso_path()}.tmp", cls.iso_path())

    @classmethod
    def create_ssh_key(cls):
        if os.path.exists(cls.ssh_key()):
            return
        os.makedirs(cls.DIR, exist_ok=True)
        subprocess.run(
            ["ssh-keygen", "-t", "ed25519", "-N", "",
             "-f", cls.ssh_key(), "-q"],
            check=True
        )

    @classmethod
    def create_disk(cls, size="20G"):
        if os.path.exists(cls.disk_path()):
            os.remove(cls.disk_path())
        os.makedirs(cls.DIR, exist_ok=True)
        subprocess.run(
            ["qemu-img", "create", "-f", "qcow2", cls.disk_path(), size],
            check=True
        )

    @classmethod
    def extract_boot(cls):
        if ARCH != "x86_64":
            return
        marker = os.path.join(cls.boot_dir(), "boot.cfg")
        if os.path.exists(marker):
            return
        os.makedirs(cls.boot_dir(), exist_ok=True)
        subprocess.run(
            ["bsdtar", "-xf", cls.iso_path(), "-C", cls.boot_dir(),
             "isolinux/isolinux.cfg"],
            check=True
        )
        cfg_path = os.path.join(
            cls.boot_dir(), "isolinux", "isolinux.cfg"
        )
        with open(cfg_path, encoding="utf-8") as f:
            cfg = f.read()
        pattern = (
            r'LABEL boot-serial\n.*?LINUX (.+?)\n'
            r'.*?APPEND (.+?)\n.*?INITRD (.+?)\n'
        )
        match = re.search(pattern, cfg, re.DOTALL)
        if not match:
            raise RuntimeError(
                "Could not find boot-serial entry in isolinux.cfg"
            )
        kernel_iso = match.group(1).strip()
        append = match.group(2).strip()
        initrd_iso = match.group(3).strip()
        subprocess.run(
            ["bsdtar", "-xf", cls.iso_path(), "-C", cls.boot_dir(),
             kernel_iso.lstrip("/"), initrd_iso.lstrip("/")],
            check=True
        )
        with open(marker, "w", encoding="utf-8") as f:
            json.dump({
                "kernel": os.path.join(
                    cls.boot_dir(), kernel_iso.lstrip("/")
                ),
                "initrd": os.path.join(
                    cls.boot_dir(), initrd_iso.lstrip("/")
                ),
                "append": append,
            }, f)

    @classmethod
    def boot_cfg(cls):
        with open(os.path.join(cls.boot_dir(), "boot.cfg"),
                  encoding="utf-8") as f:
            return json.load(f)

    # Boot
    @classmethod
    def boot(cls, from_iso=True, ram="8G", cpus=4):
        accel = ["-accel", "hvf"] if MACOS else ["-enable-kvm"]
        args = [
            f"qemu-system-{ARCH}", *accel,
            "-m", ram, "-smp", str(cpus),
            "-drive", f"file={cls.disk_path()},format=qcow2,if=virtio,cache=none",
            "-netdev", f"user,id=net0,hostfwd=tcp::{cls.SSH_PORT}-:22",
            "-device", "virtio-net-pci,netdev=net0",
            "-nographic",
        ]
        if ARCH == "aarch64":
            args += ["-machine", "virt", "-cpu", "host"]
        if from_iso:
            if ARCH == "x86_64":
                cfg = cls.boot_cfg()
                args += [
                    "-kernel", cfg["kernel"],
                    "-initrd", cfg["initrd"],
                    "-append", cfg["append"],
                    "-cdrom", cls.iso_path(),
                ]
            else:
                cls.setup_ovmf()
                args += [
                    "-drive",
                    f"if=pflash,format=raw,readonly=on,file={cls.ovmf_code()}",
                    "-drive",
                    f"if=pflash,format=raw,file={cls.ovmf_path()}",
                    "-cdrom", cls.iso_path(),
                    "-boot", "d",
                ]
        else:
            cls.setup_ovmf()
            args += [
                "-drive",
                f"if=pflash,format=raw,readonly=on,file={cls.ovmf_code()}",
                "-drive",
                f"if=pflash,format=raw,file={cls.ovmf_path()}",
            ]
        cls.output = []
        cls.process = subprocess.Popen(
            args, stdin=subprocess.PIPE,
            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL
        )
        cls.reader = threading.Thread(target=cls.read_loop, daemon=True)
        cls.reader.start()
        time.sleep(10 if not from_iso else 2)
        if cls.process.poll() is not None:
            raise RuntimeError("QEMU failed to start")

    @classmethod
    def read_loop(cls):
        assert cls.process and cls.process.stdout
        fd = cls.process.stdout.fileno()
        while cls.process and cls.process.poll() is None:
            try:
                data = os.read(fd, 4096)
                if data:
                    cls.output.append(data.decode(errors="replace"))
                else:
                    break
            except OSError:
                break

    # Serial
    @classmethod
    def send_raw(cls, data, delay=0):
        if not cls.process or not cls.process.stdin \
                or cls.process.poll() is not None:
            raise RuntimeError("QEMU process not running")
        cls.process.stdin.write(
            data.encode() if isinstance(data, str) else data
        )
        cls.process.stdin.flush()
        if delay:
            time.sleep(delay)

    @classmethod
    def serial_send(cls, cmd, delay=1):
        cls.send_raw(f"{cmd}\n", delay=delay)

    @classmethod
    def serial_read(cls):
        output = "".join(cls.output)
        cls.output.clear()
        return output

    @classmethod
    def serial_bootstrap_ssh(cls):
        pubkey = cls.ssh_pubkey()
        cls.serial_send("", delay=1)
        cls.serial_send(
            "sudo mkdir -p /root/.ssh && sudo chmod 700 /root/.ssh"
        )
        cls.serial_send(
            f"echo '{pubkey}' | sudo tee "
            "/root/.ssh/authorized_keys > /dev/null"
        )
        cls.serial_send("sudo chmod 600 /root/.ssh/authorized_keys")
        cls.serial_send("sudo systemctl restart sshd", delay=3)

    # SSH
    @classmethod
    def ssh_opts(cls):
        return [
            "-o", "StrictHostKeyChecking=no",
            "-o", "UserKnownHostsFile=/dev/null",
            "-o", "LogLevel=ERROR",
            "-o", "BatchMode=yes",
            "-i", cls.ssh_key(),
        ]

    @classmethod
    def ssh(cls, cmd, check=True, timeout=30):
        return subprocess.run(
            ["ssh", "-p", str(cls.SSH_PORT)]
            + cls.ssh_opts() + ["root@localhost", cmd],
            capture_output=True, check=check, timeout=timeout,
            encoding="utf-8", errors="replace",
        )

    @classmethod
    def ssh_ready(cls):
        try:
            return cls.ssh("true").returncode == 0
        except Exception:
            return False

    @classmethod
    def wait_for_ssh(cls, timeout=60, interval=3):
        elapsed = 0
        while elapsed < timeout:
            if cls.ssh_ready():
                return True
            time.sleep(interval)
            elapsed += interval
            if elapsed > 0 and elapsed % 15 == 0:
                cls.serial_bootstrap_ssh()
        raise TimeoutError(f"SSH not ready after {timeout}s")

    @classmethod
    def scp_to(cls, local, remote):
        return subprocess.run(
            ["scp", "-r", "-P", str(cls.SSH_PORT)]
            + cls.ssh_opts() + [local, f"root@localhost:{remote}"],
            check=True
        )

    @classmethod
    def scp_from(cls, remote, local):
        return subprocess.run(
            ["scp", "-r", "-P", str(cls.SSH_PORT)]
            + cls.ssh_opts() + [f"root@localhost:{remote}", local],
            check=True
        )

    # Lifecycle
    @classmethod
    def stop(cls):
        if cls.process and cls.process.poll() is None:
            try:
                if cls.process.stdin:
                    cls.process.stdin.write(b"\x01x")
                    cls.process.stdin.flush()
            except (BrokenPipeError, OSError):
                pass
            try:
                cls.process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                cls.process.kill()
        if cls.process:
            try:
                if cls.process.stdin:
                    cls.process.stdin.close()
            except (BrokenPipeError, OSError):
                pass
            try:
                if cls.process.stdout:
                    cls.process.stdout.close()
            except OSError:
                pass
        cls.process = None
        cls.reader = None
        cls.output = []

    # Snapshots
    @classmethod
    def ovmf_snapshot_path(cls, name):
        return os.path.join(cls.DIR, f"OVMF_VARS.{name}.fd")

    @classmethod
    def snapshot(cls, name):
        cls.stop()
        subprocess.run(
            ["qemu-img", "snapshot", "-c", name, cls.disk_path()],
            check=True
        )
        if os.path.exists(cls.ovmf_path()):
            shutil.copy2(cls.ovmf_path(), cls.ovmf_snapshot_path(name))

    @classmethod
    def restore(cls, name):
        cls.stop()
        subprocess.run(
            ["qemu-img", "snapshot", "-a", name, cls.disk_path()],
            check=True
        )
        saved = cls.ovmf_snapshot_path(name)
        if os.path.exists(saved):
            shutil.copy2(saved, cls.ovmf_path())

    @classmethod
    def has_snapshot(cls, name):
        if not os.path.exists(cls.disk_path()):
            return False
        result = subprocess.run(
            ["qemu-img", "snapshot", "-l", cls.disk_path()],
            capture_output=True, text=True, check=False
        )
        return name in result.stdout

    @classmethod
    def delete_snapshot(cls, name):
        subprocess.run(
            ["qemu-img", "snapshot", "-d", name, cls.disk_path()],
            check=True
        )
        saved = cls.ovmf_snapshot_path(name)
        if os.path.exists(saved):
            os.remove(saved)

    @classmethod
    def setup_ovmf(cls):
        if os.path.exists(cls.ovmf_path()):
            return
        os.makedirs(cls.DIR, exist_ok=True)
        code_path = cls.ovmf_code()
        vars_src = os.path.join(os.path.dirname(code_path), "OVMF_VARS.fd")
        if os.path.exists(vars_src):
            shutil.copy2(vars_src, cls.ovmf_path())
        else:
            with open(cls.ovmf_path(), "wb") as f:
                f.write(b"\x00" * 64 * 1024 * 1024)
        os.chmod(cls.ovmf_path(), 0o644)

    # Compound operations
    @classmethod
    def setup(cls):
        cls.download_iso()
        cls.create_ssh_key()
        cls.create_disk()
        cls.extract_boot()
        cls.setup_ovmf()

    @classmethod
    def up(cls):
        cls.setup()
        cls.boot(from_iso=True)
        time.sleep(20)
        cls.serial_bootstrap_ssh()
        cls.wait_for_ssh()

    CHECKPOINTS = [
        "live-ssh", "installed", "booted",
        "reset-tested", "perf-tested",
        "snapshot-only-rebuilt", "snapshot-only-tested",
        "disabled-rebuilt", "disabled-tested",
        "restore-history-ready",
        "restore-previous-rebuilt", "restore-previous-tested",
        "restore-penultimate-rebuilt", "restore-penultimate-tested",
    ]

    @classmethod
    def clean(cls):
        """Delete disk + snapshots but keep ISO and SSH keys."""
        cls.stop()
        for name in [cls.disk_path(), cls.ovmf_path()]:
            if os.path.exists(name):
                os.remove(name)
        for f in os.listdir(cls.DIR):
            if f.startswith("OVMF_VARS.") and f.endswith(".fd"):
                os.remove(os.path.join(cls.DIR, f))
        boot = cls.boot_dir()
        if os.path.exists(boot):
            shutil.rmtree(boot)

    @classmethod
    def clean_all(cls):
        """Delete everything including ISO."""
        cls.stop()
        if os.path.exists(cls.DIR):
            shutil.rmtree(cls.DIR)

    @classmethod
    def clean_iso(cls):
        """Delete only the ISO (redownload on next run)."""
        iso = cls.iso_path()
        if os.path.exists(iso):
            os.remove(iso)

    @classmethod
    def clean_from(cls, stage):
        """Delete all snapshots at or after the given stage."""
        cls.stop()
        try:
            idx = cls.CHECKPOINTS.index(stage)
        except ValueError:
            raise ValueError(
                f"Unknown stage '{stage}'. "
                f"Valid: {', '.join(cls.CHECKPOINTS)}"
            ) from None
        to_delete = cls.CHECKPOINTS[idx:]
        for name in to_delete:
            if cls.has_snapshot(name):
                cls.delete_snapshot(name)


def main():
    parser = argparse.ArgumentParser(description="NixOS VM test harness")
    parser.add_argument(
        "command",
        choices=[
            "setup", "up", "ssh", "stop", "status",
            "snapshot", "restore",
            "clean", "clean-all", "clean-iso", "clean-from",
        ],
    )
    parser.add_argument("args", nargs="*")
    args = parser.parse_args()
    match args.command:
        case "setup":
            VM.setup()
        case "up":
            VM.up()
        case "ssh":
            print(VM.ssh(" ".join(args.args)).stdout, end="")
        case "snapshot":
            name = args.args[0] if args.args else "default"
            VM.snapshot(name)
            print(f"Snapshot '{name}' created")
        case "restore":
            name = args.args[0] if args.args else "default"
            VM.restore(name)
            print(f"Snapshot '{name}' restored")
        case "stop":
            VM.stop()
        case "clean":
            VM.clean()
            print("Deleted disk + snapshots (ISO kept)")
        case "clean-all":
            VM.clean_all()
            print("Deleted everything")
        case "clean-iso":
            VM.clean_iso()
            print("Deleted ISO")
        case "clean-from":
            if not args.args:
                print("Usage: clean-from <stage>")
                print(f"Stages: {', '.join(VM.CHECKPOINTS)}")
                return
            VM.clean_from(args.args[0])
            print(f"Deleted snapshots from '{args.args[0]}' onward")
        case "status":
            iso = "yes" if os.path.exists(VM.iso_path()) else "no"
            disk = "yes" if os.path.exists(VM.disk_path()) else "no"
            key = "yes" if os.path.exists(VM.ssh_key()) else "no"
            running = VM.process and VM.process.poll() is None
            print(f"DIR:     {VM.DIR}")
            print(f"ARCH:    {ARCH}")
            print(f"ISO:     {iso}")
            print(f"Disk:    {disk}")
            print(f"SSH key: {key}")
            print(f"Running: {'yes' if running else 'no'}")
            if os.path.exists(VM.disk_path()):
                result = subprocess.run(
                    ["qemu-img", "snapshot", "-l", VM.disk_path()],
                    capture_output=True, text=True, check=False,
                )
                if result.stdout.strip():
                    print(f"Snapshots:\n{result.stdout.strip()}")


if __name__ == "__main__":
    main()
