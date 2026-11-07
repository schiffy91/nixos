"""Private, fixed-content sbctl key transfer; never generate or enroll keys."""
import ctypes
import errno
import io
import json
import os
import resource
import secrets
import signal
import stat
import subprocess
import sys
import tarfile
import uuid

MEMBERS = ("GUID",) + tuple(f"keys/{key}/{key}.{ext}"
                           for key in ("PK", "KEK", "db") for ext in ("key", "pem"))
FILE_LIMIT = 65536
ARCHIVE_LIMIT = 1024 * 1024


class BundleError(Exception):
    pass


def components(path):
    if not path.startswith("/") or path == "/" or any(p in ("", ".", "..") for p in path.split("/")[1:]):
        raise BundleError("Use an absolute bundle path without empty or relative components")
    if path == "/nix/store" or path.startswith("/nix/store/"):
        raise BundleError("Key bundles must never enter the Nix store")
    return path.split("/")[1:]


def check_directory(fd):
    info = os.fstat(fd)
    if not stat.S_ISDIR(info.st_mode) or info.st_uid != 0 or info.st_mode & 0o022:
        raise BundleError("Bundle ancestors must be root-owned directories without group/other write access")


def open_directory(parts, create=False, initial=None):
    fd = os.open("/", os.O_RDONLY | os.O_DIRECTORY) if initial is None else os.dup(initial)
    try:
        check_directory(fd)
        for part in parts:
            if create:
                try:
                    os.mkdir(part, 0o700, dir_fd=fd)
                    os.fsync(fd)
                except FileExistsError:
                    pass
            child = os.open(part, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW, dir_fd=fd)
            os.close(fd)
            fd = child
            check_directory(fd)
        return fd
    except BaseException:
        os.close(fd)
        raise


def read_bundle(path):
    bundle = open_directory(components(path))
    result = {}
    try:
        for name in MEMBERS:
            parts = name.split("/")
            parent = open_directory(parts[:-1], initial=bundle)
            try:
                fd = os.open(parts[-1], os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK, dir_fd=parent)
                with os.fdopen(fd, "rb") as stream:
                    info = os.fstat(stream.fileno())
                    mode = stat.S_IMODE(info.st_mode)
                    permitted = mode in (0o400, 0o600) if name.endswith(".key") else mode in (0o400, 0o600, 0o444, 0o644)
                    if not stat.S_ISREG(info.st_mode) or info.st_uid != 0 or info.st_nlink != 1 or not permitted:
                        raise BundleError("Unsafe key bundle file type, ownership, link count, or permissions")
                    result[name] = stream.read(FILE_LIMIT + 1)
                    if not result[name] or len(result[name]) > FILE_LIMIT:
                        raise BundleError("Empty or oversized key bundle file")
            finally:
                os.close(parent)
    finally:
        os.close(bundle)
    return result


def openssl(arguments, data):
    result = subprocess.run(["openssl", *arguments], input=data, stdout=subprocess.PIPE,
                            stderr=subprocess.DEVNULL, timeout=10)
    if result.returncode:
        raise BundleError("Invalid Secure Boot certificate or private key")
    return result.stdout


def verify(files):
    try:
        guid = str(uuid.UUID(files["GUID"].decode("ascii").strip()))
    except (UnicodeError, ValueError):
        raise BundleError("Invalid sbctl owner GUID") from None
    fingerprints = {}
    for key in ("PK", "KEK", "db"):
        private = files[f"keys/{key}/{key}.key"]
        certificate = files[f"keys/{key}/{key}.pem"]
        openssl(["pkey", "-check", "-noout", "-passin", "pass:"], private)
        private_public = openssl(["pkey", "-pubout", "-outform", "DER", "-passin", "pass:"], private)
        certificate_public = openssl(["x509", "-pubkey", "-noout"], certificate)
        certificate_public = openssl(["pkey", "-pubin", "-outform", "DER"], certificate_public)
        if private_public != certificate_public:
            raise BundleError("Secure Boot private key does not match its certificate")
        fingerprint = openssl(["x509", "-fingerprint", "-sha256", "-noout"], certificate).decode("ascii").strip()
        fingerprints[key] = fingerprint.split("=", 1)[1]
    return {"guid": guid, "certificateSha256": fingerprints}


def export_bundle(files):
    if os.isatty(sys.stdout.fileno()):
        raise BundleError("Refusing to print private keys to a terminal; pipe directly to the remote importer")
    archive = io.BytesIO()
    with tarfile.open(fileobj=archive, mode="w", format=tarfile.USTAR_FORMAT) as stream:
        for name in MEMBERS:
            member = tarfile.TarInfo(name)
            member.size = len(files[name])
            member.mode = 0o600
            stream.addfile(member, io.BytesIO(files[name]))
    sys.stdout.buffer.write(archive.getvalue())
    sys.stdout.buffer.flush()


def read_archive():
    if os.isatty(sys.stdin.fileno()):
        raise BundleError("Import requires a private archive on stdin")
    data = sys.stdin.buffer.read(ARCHIVE_LIMIT + 1)
    if len(data) > ARCHIVE_LIMIT or len(data) < 1024 or len(data) % 512 or data[-1024:] != bytes(1024):
        raise BundleError("Incomplete or oversized key archive")
    files = {}
    end = 0
    with tarfile.open(fileobj=io.BytesIO(data), mode="r:") as archive:
        for member in archive:
            if member.name not in MEMBERS or member.name in files or member.type != tarfile.REGTYPE or member.pax_headers or member.sparse:
                raise BundleError("Key archive must contain exactly the seven regular sbctl bundle files")
            if not 0 < member.size <= FILE_LIMIT:
                raise BundleError("Empty or oversized archive member")
            stream = archive.extractfile(member)
            if stream is None:
                raise BundleError("Missing archive member")
            end = member.offset_data + ((member.size + 511) // 512) * 512
            files[member.name] = stream.read(FILE_LIMIT + 1)
            if len(files[member.name]) != member.size:
                raise BundleError("Incomplete archive member")
    if any(data[end:]):
        raise BundleError("Unexpected trailing data in key archive")
    if set(files) != set(MEMBERS):
        raise BundleError("Incomplete key bundle")
    return files


def publish(files, path):
    parts = components(path)
    parent = open_directory(parts[:-1], create=True)
    staged = ".secure-boot-keys-" + secrets.token_hex(16)
    stage_fd = None
    published = False
    try:
        try:
            os.stat(parts[-1], dir_fd=parent, follow_symlinks=False)
        except FileNotFoundError:
            pass
        else:
            raise BundleError("Destination already exists; no keys were overwritten")
        os.mkdir(staged, 0o700, dir_fd=parent)
        stage_fd = open_directory([staged], initial=parent)
        for name in MEMBERS:
            relative = name.split("/")
            directory = open_directory(relative[:-1], create=True, initial=stage_fd)
            try:
                fd = os.open(relative[-1], os.O_WRONLY | os.O_CREAT | os.O_EXCL | os.O_NOFOLLOW, 0o600, dir_fd=directory)
                with os.fdopen(fd, "wb") as stream:
                    stream.write(files[name])
                    stream.flush()
                    os.fsync(stream.fileno())
                os.fsync(directory)
            finally:
                os.close(directory)
        os.fsync(stage_fd)
        # renameat2 refuses even an empty existing destination, atomically.
        libc = ctypes.CDLL(None, use_errno=True)
        rename = libc.renameat2
        rename.argtypes = (ctypes.c_int, ctypes.c_char_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_uint)
        rename.restype = ctypes.c_int
        if rename(parent, os.fsencode(staged), parent, os.fsencode(parts[-1]), 1) != 0:
            if ctypes.get_errno() == errno.EEXIST:
                raise BundleError("Destination appeared during import; no keys were overwritten")
            raise BundleError("Could not atomically publish the key bundle")
        published = True
        os.fsync(parent)
    finally:
        if stage_fd is not None:
            if not published:
                # Only our fixed files under our already-open private directory.
                for name in reversed(MEMBERS):
                    try:
                        os.unlink(name, dir_fd=stage_fd)
                    except FileNotFoundError:
                        pass
                for name in ("keys/db", "keys/KEK", "keys/PK", "keys"):
                    try:
                        os.rmdir(name, dir_fd=stage_fd)
                    except FileNotFoundError:
                        pass
                os.rmdir(staged, dir_fd=parent)
            os.close(stage_fd)
        os.close(parent)


def interrupted(_signal, _frame):
    raise BundleError("Key transfer interrupted; no incomplete destination was published")


def main():
    if os.getuid() != 0 or os.geteuid() != 0:
        raise BundleError("Secure Boot key transfer requires root")
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    os.umask(0o077)
    if ctypes.CDLL(None).prctl(4, 0, 0, 0, 0) != 0:  # PR_SET_DUMPABLE
        raise BundleError("Could not disable process dumps")
    signal.signal(signal.SIGINT, interrupted)
    signal.signal(signal.SIGTERM, interrupted)
    if len(sys.argv) != 3 or sys.argv[1] not in ("export", "import", "verify"):
        raise BundleError("Usage: nixosctl secure-boot keys <export|import|verify> /absolute/bundle/path")
    action, path = sys.argv[1:]
    components(path)
    files = read_archive() if action == "import" else read_bundle(path)
    report = verify(files)
    if action == "export":
        export_bundle(files)
    elif action == "import":
        publish(files, path)
        print("Secure Boot key bundle imported and verified.", file=sys.stderr)
    else:
        print(json.dumps(report, sort_keys=True))


if __name__ == "__main__":
    try:
        main()
    except BundleError as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
    except (OSError, ValueError, UnicodeError, tarfile.TarError, subprocess.SubprocessError):
        # Never print exception details, archive contents, key bytes, or subprocess output.
        print("Secure Boot key operation failed; check root permissions, the complete matching bundle, and an absent safe destination.", file=sys.stderr)
        sys.exit(1)
