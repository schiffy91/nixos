"""Exercise the compiled verifier against real public-key signatures."""
import gzip
import lzma
from pathlib import Path
import struct
import subprocess
import sys
import tempfile


def run(*arguments, **kwargs):
    return subprocess.run(arguments, check=True, capture_output=True, **kwargs).stdout


def main(binary):
    with tempfile.TemporaryDirectory(prefix="module-trust-unit-") as temporary:
        root = Path(temporary)
        modules = root / "package/lib/modules/test/extra"
        modules.mkdir(parents=True)
        content = root / "content"
        content.write_bytes(b"\x7fELF\0signed module fixture\xff")
        for name in ("trusted", "foreign"):
            run("openssl", "req", "-new", "-x509", "-newkey", "ec", "-pkeyopt", "ec_paramgen_curve:P-256",
                "-nodes", "-subj", f"/CN={name}", "-keyout", str(root / f"{name}.key"), "-out", str(root / f"{name}.pem"))
        certificate = run("openssl", "x509", "-in", str(root / "trusted.pem"), "-outform", "DER")
        (root / "certificates").write_bytes(certificate)
        (root / "image").write_bytes(b"image\0" + certificate + b"\xff")

        def signed(name):
            signature = run("openssl", "cms", "-sign", "-binary", "-noattr", "-in", str(content),
                            "-signer", str(root / f"{name}.pem"), "-inkey", str(root / f"{name}.key"), "-outform", "DER")
            return content.read_bytes() + signature + bytes([0, 0, 2, 0, 0, 0, 0, 0]) + struct.pack(">I", len(signature)) + b"~Module signature appended~\n"

        raw = signed("trusted")
        for suffix, data in (("", raw), (".gz", gzip.compress(raw)), (".xz", lzma.compress(raw)),
                             (".zst", run("zstd", "-cq", input=raw))):
            (modules / f"fixture{suffix.replace('.', '_')}.ko{suffix}").write_bytes(data)
            (modules / f"fixture{suffix.replace('.', '_')}.ko.cer").write_bytes(certificate)
        command = [binary, str(root / "image"), str(root / "certificates"), "test", str(root / "package")]
        assert b"4 external modules" in run(*command)
        target = modules / "fixture.ko"

        def rejected(reason):
            result = subprocess.run(command, capture_output=True)
            assert result.returncode and reason in result.stderr, result.stderr

        target.write_bytes(b"tampered" + raw[8:])
        rejected(b"signature does not match")
        target.write_bytes(signed("foreign"))
        rejected(b"signature does not match")
        target.write_bytes(content.read_bytes())
        rejected(b"unsigned module")
        target.write_bytes(raw)
        (modules / "fixture.ko.cer").unlink()
        rejected(b"missing certificate")
        (modules / "fixture.ko.cer").write_bytes(b"untrusted")
        rejected(b"absent from the kernel trust set")
        (modules / "fixture.ko.cer").write_bytes(certificate)
        (root / "image").write_bytes(b"different kernel")
        rejected(b"does not contain")
        (root / "certificates").write_bytes(certificate[:-1])
        rejected(b"truncated aggregate")
    print("PASS: real signed modules, four encodings, tampering, foreign signer, missing trust and truncated certificates")


if __name__ == "__main__":
    main(sys.argv[1])
