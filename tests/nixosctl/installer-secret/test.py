import os
from pathlib import Path
import signal
import subprocess

import pexpect

root = Path('/run/installer-test')
root.mkdir(mode=0o755)
hash_root = Path('/var/lib/installer-test/hashes')
hash_root.mkdir(parents=True, mode=0o700)


def spawn(path, mode='ok', prompt=True):
    child = pexpect.spawn('installer-secret-test', [str(path), str(hash_root / path.parent.name), mode],
                          encoding='utf-8', timeout=20)
    if prompt:
        child.expect_exact('Set your password: ')
        child.sendline('testpass123')
        child.expect_exact('Confirm password: ')
        child.sendline('testpass123')
    child.expect_exact('prepared: ')
    return child


def finish(child, success=True):
    child.sendline('continue')
    child.expect(pexpect.EOF)
    child.close()
    assert (child.exitstatus == 0) == success, child.before


def assert_private(path):
    assert path.read_bytes() == b'testpass123'
    assert path.stat().st_uid == 0 and path.stat().st_mode & 0o777 == 0o600
    assert path.parent.stat().st_mode & 0o777 == 0o700
    denied = subprocess.run(['runuser', '-u', 'observer', '--', 'cat', str(path)], capture_output=True)
    assert denied.returncode != 0 and not denied.stdout


# A permissive caller umask must not open a public-file window.
os.umask(0)
for mode in ['ok', 'fail', 'killed']:
    path = root / mode / 'passphrase'
    child = spawn(path, mode)
    assert_private(path)
    if mode == 'killed':
        child.kill(signal.SIGKILL)
        child.expect(pexpect.EOF)
        child.close()
        assert_private(path)  # still protected; /run is removed on reboot
    else:
        finish(child, success=mode == 'ok')
        assert not path.exists()

# A safely supplied file remains caller-owned and initializes the hash without
# prompting or replacing its password, even when no account hash exists yet.
path = root / 'supplied' / 'passphrase'
path.parent.mkdir(mode=0o700)
path.touch(mode=0o600)
path.write_bytes(b'testpass123')
child = spawn(path, prompt=False)
assert_private(path)
finish(child)
assert_private(path)
hashed = (hash_root / 'supplied').read_text().strip()
assert hashed.startswith('$6$')
# Verify the existing passphrase, not a fresh prompt, produced the stored hash.
salt = hashed.split('$')[2]
actual = subprocess.run(['mkpasswd', '-m', 'sha-512', '-S', salt, '-s'],
                        input='testpass123\n', text=True, capture_output=True, check=True)
assert actual.stdout.strip() == hashed
child = spawn(path, prompt=False)
finish(child)
assert (hash_root / 'supplied').read_text().strip().startswith('$6$')

# A supplied passphrase must replace a stale hash from a failed/earlier install.
(hash_root / 'supplied').write_text('!\n')
child = spawn(path, prompt=False)
finish(child)
hashed = (hash_root / 'supplied').read_text().strip()
actual = subprocess.run(['mkpasswd', '-m', 'sha-512', '-S', hashed.split('$')[2], '-s'],
                        input='testpass123\n', text=True, capture_output=True, check=True)
assert actual.stdout.strip() == hashed

for kind in ['public', 'owner', 'symlink', 'hardlink', 'parent', 'parent-link', 'leaf-owner', 'empty', 'nul', 'newline', 'carriage-return', 'oversize']:
    path = root / kind / 'passphrase'
    path.parent.mkdir(mode=0o700)
    path.touch(mode=0o600)
    path.write_bytes(b'testpass123')
    if kind == 'public': path.chmod(0o644)
    if kind == 'owner': os.chown(path, 1000, 100)
    if kind == 'symlink':
        path.unlink()
        path.symlink_to(root / 'supplied' / 'passphrase')
    if kind == 'hardlink': os.link(path, path.parent / 'alias')
    if kind == 'parent': path.parent.chmod(0o777)
    if kind == 'parent-link':
        real = root / 'parent-real'
        path.parent.rename(real)
        path.parent.symlink_to(real)
    if kind == 'leaf-owner': os.chown(path.parent, 1000, 100)
    if kind == 'empty': path.write_bytes(b'')
    if kind == 'nul': path.write_bytes(b'testpass123\0extra')
    if kind == 'newline': path.write_bytes(b'testpass123\nextra')
    if kind == 'carriage-return': path.write_bytes(b'testpass123\rextra')
    if kind == 'oversize': path.write_bytes(b'a' * 4097)
    result = subprocess.run(['installer-secret-test', str(path), str(hash_root / kind), 'ok'],
                            input='', text=True, capture_output=True, timeout=10)
    assert result.returncode != 0, kind
    assert 'prepared:' not in result.stderr, kind
    assert 'testpass123' not in result.stderr + result.stdout, kind
    assert not (hash_root / kind).exists(), kind
assert (root / 'supplied' / 'passphrase').read_bytes() == b'testpass123'
print('Private creation, caller-owned reuse, failed-operation cleanup, unsafe-file rejection passed')
