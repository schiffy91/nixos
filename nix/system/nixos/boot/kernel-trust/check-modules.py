"""Check public module certificates before publishing an enforcing kernel image."""
import gzip
import lzma
from pathlib import Path
import struct
import subprocess
import sys
import tempfile


def fail(message):
    raise SystemExit("Kernel module trust is not ready: " + message)


def certificates(data):
    result = set()
    while data:
        if len(data) < 2 or data[0] != 0x30:
            fail("invalid aggregate certificate encoding")
        length = data[1]
        header = 2
        if length & 0x80:
            count = length & 0x7F
            if not 1 <= count <= 4 or len(data) < 2 + count:
                fail("invalid aggregate certificate length")
            length = int.from_bytes(data[2:2 + count], "big")
            header += count
        end = header + length
        if end > len(data):
            fail("truncated aggregate certificate")
        result.add(data[:end])
        data = data[end:]
    return result


image, aggregate, version, *packages = sys.argv[1:]
bundle = Path(aggregate).read_bytes()
trusted = certificates(bundle)
if not bundle or bundle not in Path(image).read_bytes():
    fail("the kernel image does not contain the selected module certificates")

checked = 0
with tempfile.TemporaryDirectory(prefix="module-public-verification-") as temporary:
    scratch = Path(temporary)
    for package in packages:
        root = Path(package) / "lib/modules" / version
        modules = sorted(p for p in root.rglob("*") if p.is_file() and
                         p.name.endswith((".ko", ".ko.xz", ".ko.gz", ".ko.zst")))
        if not modules:
            fail(f"no modules found in {root}")
        for module in modules:
            extension = module.suffix
            raw = module.read_bytes()
            if extension == ".xz":
                raw = lzma.decompress(raw)
            elif extension == ".gz":
                raw = gzip.decompress(raw)
            elif extension == ".zst":
                raw = subprocess.run(["zstd", "-dc", str(module)], check=True, capture_output=True).stdout
            certificate = Path(str(module if extension == ".ko" else module.with_suffix("")) + ".cer")
            if not certificate.is_file():
                fail(f"missing certificate for {module}; use the kernel modules_install signing step")
            if certificate.read_bytes() not in trusted:
                fail(f"certificate for {module} is absent from the kernel trust set")
            marker = b"~Module signature appended~\n"
            end = len(raw) - len(marker) - 12
            if end < 0 or not raw.endswith(marker):
                fail(f"unsigned module {module}")
            signature_size = struct.unpack(">I", raw[end + 8:end + 12])[0]
            if not 0 < signature_size <= end:
                fail(f"invalid signature length for {module}")
            (scratch / "module").write_bytes(raw[:end - signature_size])
            (scratch / "signature").write_bytes(raw[end - signature_size:end])
            pem = subprocess.run(["openssl", "x509", "-inform", "DER", "-in", str(certificate)],
                                 check=True, capture_output=True).stdout
            (scratch / "certificate.pem").write_bytes(pem)
            # Trust membership is checked above; OpenSSL verifies that this
            # certificate actually authenticates these exact module bytes.
            result = subprocess.run([
                "openssl", "cms", "-verify", "-binary", "-inform", "DER",
                "-in", str(scratch / "signature"), "-content", str(scratch / "module"),
                "-certfile", str(scratch / "certificate.pem"), "-nointern", "-noverify", "-out", "/dev/null",
            ], capture_output=True)
            if result.returncode:
                fail(f"signature does not match the certificate and contents of {module}")
            checked += 1
print(f"Kernel trust verified: {len(trusted)} embedded certificates, {checked} external modules.")
