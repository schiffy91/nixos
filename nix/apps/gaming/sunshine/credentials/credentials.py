"""Explicit Sunshine credential provisioning; never called to fetch secrets at boot."""
import fcntl
import hashlib
import json
import os
from pathlib import Path
import pwd
import re
import resource
import secrets
import stat
import string
import subprocess
import sys


class CredentialError(Exception):
    pass


def validate(record):
    return (isinstance(record, dict)
            and isinstance(record.get("username"), str)
            and 0 < len(record["username"]) <= 256
            and ":" not in record["username"]
            and not any(ord(c) < 32 for c in record["username"])
            and isinstance(record.get("salt"), str) and bool(record["salt"])
            and isinstance(record.get("password"), str)
            and re.fullmatch(r"[0-9A-F]{64}", record["password"]) is not None)


def digest(password, salt):
    # Sunshine v2026.914.233613 util::hex serializes SHA256 bytes in reverse order.
    return hashlib.sha256((password + salt).encode()).digest()[::-1].hex().upper()


class Store:
    def __init__(self, home):
        self.uid = os.getuid()
        self.fd = os.open("/", os.O_DIRECTORY | os.O_RDONLY)
        for part in (Path(home) / ".config/sunshine").parts[1:]:
            if part in (".", ".."):
                raise CredentialError("Unsafe credential directory")
            try:
                os.mkdir(part, 0o700, dir_fd=self.fd)
            except FileExistsError:
                pass
            child = os.open(part, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW, dir_fd=self.fd)
            info = os.fstat(child)
            if info.st_uid not in (0, self.uid) or info.st_mode & 0o022:
                os.close(child)
                raise CredentialError("Unsafe credential directory")
            os.close(self.fd)
            self.fd = child
        if os.fstat(self.fd).st_uid != self.uid:
            raise CredentialError("Credential directory must belong to the desktop user")
        os.fchmod(self.fd, 0o700)
        self.lock = os.open(".credentials.lock", os.O_RDWR | os.O_CREAT | os.O_NOFOLLOW | os.O_NONBLOCK, 0o600, dir_fd=self.fd)
        self.check(self.lock)
        fcntl.flock(self.lock, fcntl.LOCK_EX)

    def check(self, fd, legacy=False):
        info = os.fstat(fd)
        allowed = not (info.st_mode & 0o022) if legacy else stat.S_IMODE(info.st_mode) == 0o600
        if not stat.S_ISREG(info.st_mode) or info.st_uid != self.uid or info.st_nlink != 1 or not allowed:
            raise CredentialError("Unsafe credential file ownership, type, or permissions")

    def read(self, name="credentials.json", legacy=False):
        try:
            fd = os.open(name, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK, dir_fd=self.fd)
        except FileNotFoundError:
            return None
        with os.fdopen(fd, "rb") as stream:
            self.check(stream.fileno(), legacy)
            data = stream.read(4 * 1024 * 1024 + 1)
        if len(data) > 4 * 1024 * 1024:
            raise CredentialError("Credential file is too large")
        try:
            return json.loads(data)
        except (ValueError, UnicodeError):
            raise CredentialError("Invalid credential JSON") from None

    def write(self, record):
        if not validate(record):
            raise CredentialError("Invalid Sunshine credentials")
        self.read()  # Refuse symlinks, hardlinks, or unsafe existing authority.
        name = ".credentials-" + secrets.token_hex(16)
        fd = os.open(name, os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600, dir_fd=self.fd)
        try:
            with os.fdopen(fd, "w") as stream:
                json.dump(record, stream)
                stream.write("\n")
                stream.flush()
                os.fsync(stream.fileno())
            os.rename(name, "credentials.json", src_dir_fd=self.fd, dst_dir_fd=self.fd)
            os.fsync(self.fd)
        finally:
            try:
                os.unlink(name, dir_fd=self.fd)
            except FileNotFoundError:
                pass

    def provision(self, username, password):
        if not isinstance(username, str) or not isinstance(password, str) or not password or "\0" in password:
            raise CredentialError("Empty or invalid credential input")
        old = self.read()
        if validate(old) and old["username"] == username and old["password"] == digest(password, old["salt"]):
            return
        salt = "".join(secrets.choice(string.ascii_letters + string.digits) for _ in range(16))
        self.write({"username": username, "salt": salt, "password": digest(password, salt)})


def read_reference(reference):
    if not reference.startswith("op://"):
        raise CredentialError("Expected a 1Password secret reference")
    result = subprocess.run(["/run/wrappers/bin/op", "read", "--no-newline", reference],
                            stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, timeout=120)
    if result.returncode != 0:
        raise CredentialError("1Password retrieval failed; existing Sunshine credentials were kept")
    return result.stdout.decode()


def main():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    os.umask(0o077)
    config = json.loads(Path(sys.argv[1]).read_text())
    account = pwd.getpwnam(config["user"])
    if os.getuid() != account.pw_uid:
        raise CredentialError("Run this command as the configured desktop user")
    if len(sys.argv) != 3 or sys.argv[2] not in ("sync", "provision", "prepare", "status"):
        raise CredentialError("Usage: nixosctl sunshine <sync|provision|status>")
    action = sys.argv[2]
    if action == "sync":
        username = read_reference(config["usernameReference"])
        password = read_reference(config["passwordReference"])
        Store(config["home"]).provision(username, password)
    elif action == "provision":
        data = sys.stdin.buffer.read(32769)
        if len(data) > 32768:
            raise CredentialError("Credential input is too large")
        record = json.loads(data)
        Store(config["home"]).provision(record["username"], record["password"])
    else:
        store = Store(config["home"])
        current = store.read()
        if action == "prepare" and current is None:
            legacy = store.read("sunshine_state.json", legacy=True)
            if validate(legacy):
                current = {key: legacy[key] for key in ("username", "salt", "password")}
                store.write(current)
        if current is None and config.get("publicKeys"):
            if action == "status":
                print("Sunshine SSH-key administration is configured.")
            return
        if not validate(current):
            raise CredentialError("Sunshine credentials are not provisioned; run nixosctl sunshine sync")
    if action != "prepare":
        print("Sunshine credentials are provisioned." if action == "status" else "Sunshine credentials updated. Restart Sunshine to apply them.")


if __name__ == "__main__":
    try:
        main()
    except (CredentialError, OSError, ValueError, KeyError, TypeError, subprocess.SubprocessError) as error:
        # Do not include raw JSON, subprocess output, or secret values in errors.
        print(str(error) if isinstance(error, CredentialError) else "Sunshine credential operation failed", file=sys.stderr)
        sys.exit(1)
