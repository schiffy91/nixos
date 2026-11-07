import concurrent.futures
import os
from pathlib import Path
import shlex
import subprocess
import sys

import pexpect


def run(command, success=True):
    result = subprocess.run(command, shell=True, text=True, capture_output=True, timeout=90)
    assert (result.returncode == 0) == success, (command, result.stdout, result.stderr)
    return result.stdout.strip()


def authenticate(password, success=True):
    child = pexpect.spawn("runuser", ["--pty", "-u", "admin", "--", "/bin/sh", "-c", "sudo -k true"], encoding="utf-8", timeout=20)
    child.expect(r"\[sudo\] password for admin: ")
    child.sendline(password)
    child.expect(pexpect.EOF)
    child.close()
    assert (child.exitstatus == 0) == success, child.before


target = Path("/var/lib/nixos/credentials/password-hash")
helper = "nixosctl-password-file"

if len(sys.argv) > 1:
    authenticate("testpass123")
    authenticate("attacker", success=False)
    assert target.read_bytes() == Path("/var/tmp/original-hash").read_bytes()
    assert run("stat -c '%u:%g:%a' /var/lib/nixos/credentials") == "0:0:700"
    assert run(f"stat -c '%u:%g:%a' {target}") == "0:0:600"
    print("password and protected file survived reboot")
    sys.exit(0)

authenticate("testpass123")
assert not target.exists()
# The legacy file is attacker-controlled. Migration must preserve shadow instead.
run("chown -R admin:users /etc/nixos")
run("echo attacker | mkpasswd -m sha-512 -s > /etc/nixos/secrets/hashed_password.txt")
protected = run("readlink -f /run/current-system/specialisation/protected")
run(f"{protected}/bin/switch-to-configuration test")
authenticate("testpass123")
authenticate("attacker", success=False)
original = target.read_bytes()
Path("/var/tmp/original-hash").write_bytes(original)
os.chmod("/var/tmp/original-hash", 0o600)
for command in [f"cat {target}", f"rm {target}", f"mv {target.parent} /tmp/stolen", f"echo bogus > {target}"]:
    run(f"runuser -u admin -- sh -c {shlex.quote(command)}", success=False)

# Existing state wins over subsequent shadow changes; initialization is idempotent.
run("echo 'admin:changed-shadow-only' | chpasswd")
with concurrent.futures.ThreadPoolExecutor(max_workers=6) as executor:
    list(executor.map(run, [f"{helper} {target} admin"] * 12))
assert target.read_bytes() == original
run(f"{protected}/bin/switch-to-configuration test")
authenticate("testpass123")

fixtures = Path("/var/lib/password-file-test")
fixtures.mkdir(mode=0o755)
for kind in ["writable-parent", "owned-parent", "symlink-parent", "symlink-leaf", "writable-leaf", "owned-leaf", "symlink-file", "hardlink-file", "public-file", "owned-file", "invalid-hash", "nul-hash", "missing-account"]:
    parent = fixtures / kind
    parent.mkdir(mode=0o755)
    leaf = parent / "private"
    leaf.mkdir(mode=0o700)
    path = leaf / "hash"
    path.write_bytes(original)
    path.chmod(0o600)
    if kind == "writable-parent": parent.chmod(0o777)
    if kind == "owned-parent": os.chown(parent, 1000, 100)
    if kind in ["symlink-parent", "symlink-leaf"]:
        link = parent if kind == "symlink-parent" else leaf
        moved = link.with_name(link.name + ".real")
        link.rename(moved)
        link.symlink_to(moved)
    if kind == "writable-leaf": leaf.chmod(0o750)
    if kind == "owned-leaf": os.chown(leaf, 1000, 100)
    if kind == "symlink-file":
        path.unlink()
        path.symlink_to(target)
    if kind == "hardlink-file": os.link(path, leaf / "alias")
    if kind == "public-file": path.chmod(0o644)
    if kind == "owned-file": os.chown(path, 1000, 100)
    if kind == "invalid-hash": path.write_bytes(b"not-a-hash\n")
    if kind == "nul-hash": path.write_bytes(original.rstrip() + b"\0extra\n")
    if kind == "missing-account": path.unlink()
    account = "does-not-exist" if kind == "missing-account" else "admin"
    run(f"{helper} {path} {account}", success=False)
    assert target.read_bytes() == original

# Activation must fail before users are updated if the authority is unsafe.
target.chmod(0o644)
run(f"{protected}/activate", success=False)
authenticate("testpass123")
target.chmod(0o600)
run(f"nix-env -p /nix/var/nix/profiles/system --set {protected}")
run(f"{protected}/bin/switch-to-configuration boot")
print("migration, authentication, concurrency, permissions, symlink and activation failures passed")
