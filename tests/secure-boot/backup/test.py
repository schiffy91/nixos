import json
import shlex

start_all()
for node in (deck, fractal):
    node.wait_for_unit("multi-user.target")
    node.wait_for_unit("sshd")
    node.succeed("findmnt -n -o SOURCE /var/lib/secureboot-fixture | grep /dev/mapper/fixture")
    node.fail("command -v nixosctl")
    node.fail("/run/current-system/sw/bin/nixosctl-secure-boot-keys")

root = "/var/lib/secureboot-fixture"
source = root + "/sbctl"
backup = root + "/backups/STEAM-DECK-OLED"
restored = root + "/restored"
archive = root + "/archive.tar"
cli = "$(cat /etc/fixture-cli-path) secure-boot keys"
ssh = "env -u SSH_AUTH_SOCK ssh -T -i /root/transfer-key -o BatchMode=yes -o IdentityAgent=none -o IdentitiesOnly=yes -o StrictHostKeyChecking=yes root@fractal"

with subtest("fresh unique fixture keys, separately encrypted storage, and pinned SSH"):
    deck.succeed("sbctl --config /etc/fixture-sbctl.conf create-keys")
    original = json.loads(deck.succeed(f"{cli} verify {source}"))
    deck.succeed("install -d -m700 /root/.ssh; ssh-keygen -q -t ed25519 -N '' -f /root/transfer-key")
    client_key = deck.succeed("cat /root/transfer-key.pub").strip()
    host_key = fractal.succeed("cat /etc/ssh/ssh_host_ed25519_key.pub").strip()
    fractal.succeed("install -d -m700 /root/.ssh; umask 077; printf '%s\\n' " + shlex.quote(client_key) + " > /root/.ssh/authorized_keys")
    deck.succeed("umask 077; printf '%s\\n' " + shlex.quote("fractal " + host_key) + " > /root/.ssh/known_hosts")
    deck.succeed(ssh + " true")

with subtest("actual CLI streams a verified backup without a local archive or secret output"):
    deck.succeed(f"set -o pipefail; {cli} export {source} | {ssh} {shlex.quote(cli + ' import ' + backup)}")
    assert json.loads(fractal.succeed(f"{cli} verify {backup}")) == original
    fractal.succeed(f"test $(stat -c %a {backup}) = 700; test $(find {backup} -type f | wc -l) = 7")
    fractal.fail(f"find {backup} -type f ! -perm 0600 | grep .")
    fractal.fail(f"find {backup} ! -user root | grep .")
    fractal.fail(f"runuser -u observer -- cat {backup}/keys/PK/PK.key")
    deck.fail(f"runuser -u observer -- {cli} export {source}")
    deck.fail(f"script -qec {shlex.quote(cli + ' export ' + source)} /dev/null > {root}/terminal-result 2>&1")
    deck.fail(f"grep -a 'BEGIN .*PRIVATE KEY' {root}/terminal-result")

with subtest("restore preserves all seven original files and existing-key creation does not rotate"):
    deck.succeed(f"set -o pipefail; {ssh} {shlex.quote(cli + ' export ' + backup)} | {cli} import {restored}")
    deck.succeed(f"for file in GUID keys/PK/PK.key keys/PK/PK.pem keys/KEK/KEK.key keys/KEK/KEK.pem keys/db/db.key keys/db/db.pem; do cmp -s {source}/$file {restored}/$file; done")
    assert json.loads(deck.succeed(f"{cli} verify {restored}")) == original
    # Pinned sbctl returns success when the existing bundle is reused.
    deck.succeed("sbctl --config /etc/fixture-sbctl.conf create-keys")
    deck.succeed(f"for file in GUID keys/PK/PK.key keys/PK/PK.pem keys/KEK/KEK.key keys/KEK/KEK.pem keys/db/db.key keys/db/db.pem; do cmp -s {source}/$file {restored}/$file; done")
    assert json.loads(deck.succeed(f"{cli} verify {source}")) == original
    deck.fail(f"set -o pipefail; {cli} export {source} | {ssh} {shlex.quote(cli + ' import ' + backup)}")
    assert json.loads(fractal.succeed(f"{cli} verify {backup}")) == original

with subtest("unsafe export sources are refused before writing any private bytes"):
    deck.succeed(f"ln -s {source} {root}/source-link")
    deck.fail(f"{cli} export {root}/source-link > {root}/refused")
    deck.succeed(f"test ! -s {root}/refused; cp -a {source} {root}/bad")
    deck.succeed(f"rm {root}/bad/keys/PK/PK.key; ln -s {source}/keys/PK/PK.key {root}/bad/keys/PK/PK.key")
    deck.fail(f"{cli} export {root}/bad > {root}/refused")
    deck.succeed(f"test ! -s {root}/refused; rm {root}/bad/keys/PK/PK.key; cp {source}/keys/PK/PK.key {root}/bad/keys/PK/PK.key; chmod 0644 {root}/bad/keys/PK/PK.key")
    deck.fail(f"{cli} export {root}/bad > {root}/refused")
    deck.succeed(f"test ! -s {root}/refused; chmod 0600 {root}/bad/keys/PK/PK.key; ln {root}/bad/keys/PK/PK.key {root}/hardlink")
    deck.fail(f"{cli} export {root}/bad > {root}/refused")
    deck.succeed(f"test ! -s {root}/refused; rm {root}/hardlink; cp {source}/keys/KEK/KEK.pem {root}/bad/keys/PK/PK.pem")
    deck.fail(f"{cli} export {root}/bad > {root}/refused")
    deck.succeed(f"test ! -s {root}/refused")

with subtest("unsafe destinations and truncated streams never publish a bundle"):
    deck.succeed(f"umask 077; {cli} export {source} > {archive}")
    deck.succeed(f"ln -s {source} {root}/destination-link; mkdir -m0777 {root}/writable")
    for destination in (root + "/destination-link", root + "/destination-link/child", root + "/writable/child", "/nix/store/refused-private-keys"):
        deck.fail(f"{cli} import {destination} < {archive}")
    deck.fail(f"head -c 4096 {archive} | {cli} import {root}/truncated")
    deck.fail(f"test -e {root}/truncated")
    # Terminate an importer while its private input pipe is incomplete.
    deck.succeed(f"mkfifo -m600 {root}/input; (head -c 512 {archive}; sleep 30) > {root}/input & writer=$!; {cli} import {root}/interrupted < {root}/input >/dev/null 2>{root}/interrupt-result & reader=$!; sleep 1; kill -TERM $reader; wait $reader && exit 1; kill $writer 2>/dev/null || true; rm {root}/input")
    deck.fail(f"test -e {root}/interrupted")
    deck.fail(f"find {root} -maxdepth 1 -name '.secure-boot-keys-*' | grep .")
    assert json.loads(deck.succeed(f"{cli} verify {source}")) == original
    assert json.loads(fractal.succeed(f"{cli} verify {backup}")) == original

with subtest("fixed-member validation rejects traversal, links, duplicates, and corrupt keys"):
    script = r'''
import io, pathlib, tarfile
root = pathlib.Path("/var/lib/secureboot-fixture")
original = root / "archive.tar"
with tarfile.open(original) as archive:
    entries = [(member, archive.extractfile(member).read()) for member in archive]
for case in ("traversal", "symlink", "duplicate", "corrupt"):
    with tarfile.open(root / (case + ".tar"), "w", format=tarfile.USTAR_FORMAT) as archive:
        for index, (member, data) in enumerate(entries):
            info = tarfile.TarInfo(member.name)
            if index == 0 and case == "traversal": info.name = "../escaped"
            if index == 0 and case == "symlink": info.type = tarfile.SYMTYPE; info.linkname = "/etc/shadow"
            if member.name.endswith(".key") and case == "corrupt": data = b"invalid-private-key"
            info.size = len(data) if info.isreg() else 0
            archive.addfile(info, io.BytesIO(data) if info.isreg() else None)
            if index == 0 and case == "duplicate": archive.addfile(info, io.BytesIO(data))
'''
    deck.succeed("umask 077; python3 - <<'PYTEST'\n" + script + "\nPYTEST")
    for case in ("traversal", "symlink", "duplicate", "corrupt"):
        deck.fail(f"{cli} import {root}/invalid-{case} < {root}/{case}.tar")
        deck.fail(f"test -e {root}/invalid-{case}")
    deck.fail("test -e /var/lib/escaped")

with subtest("failed storage writes leave no published or partial destination"):
    deck.succeed(f"mkdir -m700 {root}/full; mount -t tmpfs -o size=8k,mode=700 tmpfs {root}/full")
    deck.fail(f"{cli} import {root}/full/backup < {archive}")
    deck.fail(f"test -e {root}/full/backup")
    deck.fail(f"find {root}/full -mindepth 1 | grep .")
    deck.succeed(f"umount {root}/full")
