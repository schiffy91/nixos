#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3 qemu libarchive openssh OVMF
import argparse
import json
import platform
import re
import shutil
import subprocess
import threading
import time
from os import read as os_read
from pathlib import Path

MACOS = platform.system() == "Darwin"
ARCH = "aarch64" if platform.machine() == "arm64" else "x86_64"

class VM:
    DIR = Path(__file__).resolve().parent.parent / ".vm"
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
        return cls.DIR / "iso" / "nixos-minimal.iso"
    @classmethod
    def disk_path(cls):
        return cls.DIR / "disk.qcow2"
    @classmethod
    def ovmf_path(cls):
        return cls.DIR / "OVMF_VARS.fd"
    @classmethod
    def ovmf_code(cls):
        if MACOS:
            brew = Path(f"/opt/homebrew/share/qemu/edk2-{ARCH}-code.fd")
            if brew.exists():
                return brew
        result = subprocess.run(
            ["find", "/nix/store", "-maxdepth", "3",
             "-name", "OVMF_CODE.fd", "-path", "*OVMF*fd*"],
            capture_output=True, text=True, check=False
        )
        paths = result.stdout.strip().split("\n")
        if paths and paths[0]:
            return Path(paths[0])
        raise FileNotFoundError("UEFI firmware not found")
    @classmethod
    def boot_dir(cls):
        return cls.DIR / "boot"
    @classmethod
    def ssh_key(cls):
        return cls.DIR / "id_ed25519"
    @classmethod
    def ssh_pubkey(cls):
        return Path(f"{cls.ssh_key()}.pub").read_text(encoding="utf-8").strip()
    # Setup
    @classmethod
    def download_iso(cls):
        iso = cls.iso_path()
        if iso.exists():
            return
        iso.parent.mkdir(parents=True, exist_ok=True)
        tmp = Path(f"{iso}.tmp")
        subprocess.run(["curl", "-L", "-o", str(tmp), cls.ISO_URL], check=True)
        tmp.rename(iso)
    @classmethod
    def create_ssh_key(cls):
        if cls.ssh_key().exists():
            return
        cls.DIR.mkdir(parents=True, exist_ok=True)
        subprocess.run(
            ["ssh-keygen", "-t", "ed25519", "-N", "",
             "-f", str(cls.ssh_key()), "-q"],
            check=True
        )
    @classmethod
    def create_disk(cls, size="20G"):
        disk = cls.disk_path()
        if disk.exists():
            disk.unlink()
        cls.DIR.mkdir(parents=True, exist_ok=True)
        subprocess.run(
            ["qemu-img", "create", "-f", "qcow2", str(disk), size],
            check=True
        )
    @classmethod
    def extract_boot(cls):
        if ARCH != "x86_64":
            return
        marker = cls.boot_dir() / "boot.cfg"
        if marker.exists():
            return
        cls.boot_dir().mkdir(parents=True, exist_ok=True)
        subprocess.run(
            ["bsdtar", "-xf", str(cls.iso_path()), "-C", str(cls.boot_dir()),
             "isolinux/isolinux.cfg"],
            check=True
        )
        cfg_path = cls.boot_dir() / "isolinux" / "isolinux.cfg"
        cfg = cfg_path.read_text(encoding="utf-8")
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
            ["bsdtar", "-xf", str(cls.iso_path()), "-C", str(cls.boot_dir()),
             kernel_iso.lstrip("/"), initrd_iso.lstrip("/")],
            check=True
        )
        marker.write_text(json.dumps({
            "kernel": str(cls.boot_dir() / kernel_iso.lstrip("/")),
            "initrd": str(cls.boot_dir() / initrd_iso.lstrip("/")),
            "append": append,
        }), encoding="utf-8")
    @classmethod
    def boot_cfg(cls):
        return json.loads((cls.boot_dir() / "boot.cfg").read_text(encoding="utf-8"))
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
                    "-cdrom", str(cls.iso_path()),
                ]
            else:
                cls.setup_ovmf()
                args += [
                    "-drive",
                    f"if=pflash,format=raw,readonly=on,file={cls.ovmf_code()}",
                    "-drive",
                    f"if=pflash,format=raw,file={cls.ovmf_path()}",
                    "-cdrom", str(cls.iso_path()),
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
                data = os_read(fd, 4096)
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
            "-i", str(cls.ssh_key()),
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
        return cls.DIR / f"OVMF_VARS.{name}.fd"
    @classmethod
    def snapshot(cls, name):
        cls.stop()
        subprocess.run(
            ["qemu-img", "snapshot", "-c", name, str(cls.disk_path())],
            check=True
        )
        ovmf = cls.ovmf_path()
        if ovmf.exists():
            shutil.copy2(ovmf, cls.ovmf_snapshot_path(name))
    @classmethod
    def restore(cls, name):
        cls.stop()
        subprocess.run(
            ["qemu-img", "snapshot", "-a", name, str(cls.disk_path())],
            check=True
        )
        saved = cls.ovmf_snapshot_path(name)
        if saved.exists():
            shutil.copy2(saved, cls.ovmf_path())
    @classmethod
    def has_snapshot(cls, name):
        if not cls.disk_path().exists():
            return False
        result = subprocess.run(
            ["qemu-img", "snapshot", "-l", str(cls.disk_path())],
            capture_output=True, text=True, check=False
        )
        return name in result.stdout
    @classmethod
    def delete_snapshot(cls, name):
        subprocess.run(
            ["qemu-img", "snapshot", "-d", name, str(cls.disk_path())],
            check=True
        )
        saved = cls.ovmf_snapshot_path(name)
        if saved.exists():
            saved.unlink()
    @classmethod
    def setup_ovmf(cls):
        ovmf = cls.ovmf_path()
        if ovmf.exists():
            return
        cls.DIR.mkdir(parents=True, exist_ok=True)
        code_path = cls.ovmf_code()
        vars_src = code_path.parent / "OVMF_VARS.fd"
        if vars_src.exists():
            shutil.copy2(vars_src, ovmf)
        else:
            ovmf.write_bytes(b"\x00" * 64 * 1024 * 1024)
        ovmf.chmod(0o644)
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
        cls.stop()
        for p in [cls.disk_path(), cls.ovmf_path()]:
            if p.exists():
                p.unlink()
        for f in cls.DIR.glob("OVMF_VARS.*.fd"):
            f.unlink()
        boot = cls.boot_dir()
        if boot.exists():
            shutil.rmtree(boot)
    @classmethod
    def clean_all(cls):
        cls.stop()
        if cls.DIR.exists():
            shutil.rmtree(cls.DIR)
    @classmethod
    def clean_iso(cls):
        iso = cls.iso_path()
        if iso.exists():
            iso.unlink()
    @classmethod
    def clean_from(cls, stage):
        cls.stop()
        try:
            idx = cls.CHECKPOINTS.index(stage)
        except ValueError:
            raise ValueError(
                f"Unknown stage '{stage}'. "
                f"Valid: {', '.join(cls.CHECKPOINTS)}"
            ) from None
        for name in cls.CHECKPOINTS[idx:]:
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
            iso = "yes" if VM.iso_path().exists() else "no"
            disk = "yes" if VM.disk_path().exists() else "no"
            key = "yes" if VM.ssh_key().exists() else "no"
            running = VM.process and VM.process.poll() is None
            print(f"DIR:     {VM.DIR}")
            print(f"ARCH:    {ARCH}")
            print(f"ISO:     {iso}")
            print(f"Disk:    {disk}")
            print(f"SSH key: {key}")
            print(f"Running: {'yes' if running else 'no'}")
            if VM.disk_path().exists():
                result = subprocess.run(
                    ["qemu-img", "snapshot", "-l", str(VM.disk_path())],
                    capture_output=True, text=True, check=False,
                )
                if result.stdout.strip():
                    print(f"Snapshots:\n{result.stdout.strip()}")

if __name__ == "__main__":
    main()
