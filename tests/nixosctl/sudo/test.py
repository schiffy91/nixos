import concurrent.futures
import os
from pathlib import Path
import shlex
import socket
import subprocess
import time

import pexpect


def run(command, user=None, success=True):
    if user:
        command = f"su -s /bin/sh {user} -c {shlex.quote(command)}"
    result = subprocess.run(command, shell=True, text=True, capture_output=True)
    assert (result.returncode == 0) == success, (command, result.stdout, result.stderr)
    return result.stdout


def authenticate(command, password="testpass123", success=True):
    child = pexpect.spawn("runuser", ["--pty", "-u", "admin", "--", "/bin/sh", "-c", command], encoding="utf-8", timeout=20)
    child.expect(r"\[sudo\] password for admin: ")
    child.sendline(password)
    child.expect(pexpect.EOF)
    child.close()
    assert (child.exitstatus == 0) == success, child.before


grant = Path("/run/nixosctl-sudo/grant")
assert "No temporary" in run("nixosctl sudo status", "admin")
run("sudo -n -k true", "admin", success=False)
run("touch /run/nixosctl-sudo/grant", "admin", success=False)
for arguments in ["", "enable", "enable --for 0s", "enable --for 25h",
                  "enable --for 999999999999h", "enable --for 08h",
                  "enable --for 1h --extra", "status --extra", "disable --extra",
                  "enable --for '1h; touch /tmp/injected'"]:
    run(f"nixosctl sudo {arguments}", "admin", success=False)
assert not Path("/tmp/injected").exists()
assert not grant.exists()

authenticate("nixosctl sudo enable --for 1h", "incorrect", success=False)
assert not grant.exists()
authenticate("nixosctl sudo enable --for 1h")
assert "expires" in run("nixosctl sudo status", "admin")
assert run("sudo -n -k id -u", "admin").strip() == "0"
run("sudo -n -k true", "agent", success=False)
run("echo bogus > /run/nixosctl-sudo/grant", "admin", success=False)
run("sudo -n -k /run/current-system/sw/bin/nixosctl-sudo enable --for 2h", "admin", success=False)
original = grant.read_bytes()
authenticate("nixosctl sudo enable --for 2h", "incorrect", success=False)
assert grant.read_bytes() == original
authenticate("nixosctl sudo enable --for 2h")
assert grant.read_bytes() != original

# Exercise real simultaneous policy writers, including visudo and atomic rename.
with concurrent.futures.ThreadPoolExecutor(max_workers=6) as executor:
    list(executor.map(run, ["nixosctl sudo enable --for 1h"] * 12))
run("visudo -c")
assert sorted(p.name for p in grant.parent.iterdir()) == [".lock", "grant"]
assert grant.stat().st_uid == 0 and grant.stat().st_mode & 0o777 == 0o440
run("nixosctl sudo disable")
run("sudo -n -k true", "admin", success=False)
assert "No temporary" in run("nixosctl sudo status", "admin")

run("nixosctl sudo enable --for 2s")
time.sleep(3)
assert grant.exists()  # Expiration works without deleting the policy file.
run("sudo -n -k true", "admin", success=False)
assert "expired" in run("nixosctl sudo status", "admin")
authenticate("sudo -k true")  # Expiry preserves ordinary administration.
run("nixosctl sudo disable")

directory = grant.parent
directory.chmod(0o777)
run("nixosctl sudo enable --for 1h", success=False)
assert not grant.exists()
directory.chmod(0o755)
directory.rename("/run/nixosctl-sudo.saved")
directory.symlink_to("/run/nixosctl-sudo.saved")
run("nixosctl sudo enable --for 1h", success=False)
directory.unlink()
Path("/run/nixosctl-sudo.saved").rename(directory)

for host, expected in [("trusted.example", "yes"), ("TRUSTED.example", "yes"), ("192.0.2.1", "yes"),
                       ("untrusted.example", "no"), ("github.com", "no")]:
    config = run(f"ssh -G {host}", "admin")
    assert f"forwardagent {expected}\n" in config, config
    assert "identityagent /home/admin/.1password/agent.sock\n" in config, config

with socket.socket(socket.AF_UNIX) as forwarded:
    forwarded.bind("/tmp/forwarded-agent.sock")
    os.chmod("/tmp/forwarded-agent.sock", 0o666)
    remote = run("SSH_CONNECTION='192.0.2.1 1000 192.0.2.2 22' SSH_AUTH_SOCK=/tmp/forwarded-agent.sock ssh -G github.com", "admin")
    assert "identityagent SSH_AUTH_SOCK\n" in remote, remote
    assert "forwardagent no\n" in remote, remote
    local = run("SSH_AUTH_SOCK=/tmp/forwarded-agent.sock ssh -G github.com", "admin")
    assert "identityagent /home/admin/.1password/agent.sock\n" in local, local
    root = run("SSH_AUTH_SOCK=/tmp/forwarded-agent.sock ssh -G github.com")
    assert "identityagent SSH_AUTH_SOCK\n" in root, root
    assert "forwardagent no\n" in root, root

print("sudo authentication, renewal, revocation, expiration, isolation and SSH defaults passed")
