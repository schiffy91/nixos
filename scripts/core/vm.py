#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3 qemu libarchive OVMF
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
        "latest-nixos-minimal-x86_64-linux.iso"
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
        result = subprocess.run(
            ["find", "/nix/store", "-maxdepth", "3",
             "-name", "OVMF_CODE.fd", "-path", "*OVMF*fd*"],
            capture_output=True, text=True, check=False
        )
        paths = result.stdout.strip().split("\n")
        if paths and paths[0]:
            return paths[0]
        raise FileNotFoundError("OVMF_CODE.fd not found in /nix/store")

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
        if from_iso:
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
            capture_output=True, text=True, check=check, timeout=timeout
        )

    @classmethod
    def ssh_ready(cls):
        try:
            return cls.ssh("true").returncode == 0
        except Exception:
            return False

    @classmethod
    def wait_for_ssh(cls, timeout=180, interval=5):
        elapsed = 0
        while elapsed < timeout:
            if cls.ssh_ready():
                return True
            time.sleep(interval)
            elapsed += interval
            if elapsed > 0 and elapsed % 30 == 0:
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
    def snapshot(cls, name):
        cls.stop()
        subprocess.run(
            ["qemu-img", "snapshot", "-c", name, cls.disk_path()],
            check=True
        )

    @classmethod
    def restore(cls, name):
        cls.stop()
        subprocess.run(
            ["qemu-img", "snapshot", "-a", name, cls.disk_path()],
            check=True
        )

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

    @classmethod
    def setup_ovmf(cls):
        if os.path.exists(cls.ovmf_path()):
            return
        os.makedirs(cls.DIR, exist_ok=True)
        src = os.path.join(os.path.dirname(cls.ovmf_code()), "OVMF_VARS.fd")
        shutil.copy2(src, cls.ovmf_path())
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
        time.sleep(30)
        cls.serial_bootstrap_ssh()
        cls.wait_for_ssh(timeout=120)

    @classmethod
    def clean(cls):
        cls.stop()
        if os.path.exists(cls.DIR):
            shutil.rmtree(cls.DIR)


def main():
    parser = argparse.ArgumentParser(description="NixOS VM test harness")
    parser.add_argument(
        "command",
        choices=[
            "setup", "up", "ssh", "stop", "clean", "status",
            "snapshot", "restore",
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
        case "status":
            print(f"DIR:     {VM.DIR}")
            print(
                f"ISO:     {
                    'yes' if os.path.exists(
                        VM.iso_path()) else 'no'}")
            print(
                f"Disk:    {
                    'yes' if os.path.exists(
                        VM.disk_path()) else 'no'}")
            print(
                f"SSH key: {
                    'yes' if os.path.exists(
                        VM.ssh_key()) else 'no'}")
            running = VM.process and VM.process.poll() is None
            print(f"Running: {'yes' if running else 'no'}")


if __name__ == "__main__":
    main()
