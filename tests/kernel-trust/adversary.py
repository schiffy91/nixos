"""Adversarial public-key/module fixtures, entirely inside the VM."""
import json
import lzma
import os
from pathlib import Path
import struct
import subprocess
import tempfile

os.umask(0o077)
paths = json.loads(Path("/etc/fixture-paths").read_text())
version = os.uname().release
module = next(Path(paths["trusted"]).rglob("trusted_probe.ko.xz"))
certificate = module.with_suffix("").with_suffix(".ko.cer").read_bytes()
signed = lzma.decompress(module.read_bytes())
marker = b"~Module signature appended~\n"
end = len(signed) - len(marker) - 12
signature_size = struct.unpack(">I", signed[end + 8:end + 12])[0]
unsigned = signed[:end - signature_size]


def run(*args):
    return subprocess.run(args, check=True, capture_output=True).stdout


with tempfile.TemporaryDirectory(prefix="module-adversary-", dir="/run") as temporary:
    root = Path(temporary)
    package = root / "package"
    modules = package / "lib/modules" / version / "extra"
    modules.mkdir(parents=True)
    target = modules / "trusted_probe.ko"
    cert = modules / "trusted_probe.ko.cer"
    bundle = root / "modules.cer"
    image = root / "image"
    bundle.write_bytes(certificate)
    image.write_bytes(b"fixture image " + certificate)
    target.write_bytes(signed)
    cert.write_bytes(certificate)
    command = ["/etc/check-module-trust", str(image), str(bundle), version, str(package)]
    run(*command)

    def rejected(reason):
        result = subprocess.run(command, capture_output=True)
        assert result.returncode != 0 and reason.encode() in result.stderr, result.stderr

    cert.unlink()
    rejected("missing certificate")
    cert.write_bytes(certificate)

    # Re-running the public algorithm on changed code produces a different
    # public key, not a way to sign arbitrary code under the original key.
    changed = bytearray(unsigned)
    changed[changed.index(b"module trust fixture loaded")] ^= 1
    target.write_bytes(changed)
    run("kmodigest", str(target))
    original_cert = root / "original.cer"
    original_cert.write_bytes(certificate)
    original_key = run("openssl", "x509", "-inform", "DER", "-in", str(original_cert), "-pubkey", "-noout")
    changed_key = run("openssl", "x509", "-inform", "DER", "-in", str(cert), "-pubkey", "-noout")
    assert original_key != changed_key
    result = subprocess.run(["insmod", str(target)], capture_output=True)
    assert result.returncode != 0 and b"Key was rejected" in result.stderr, result.stderr
    rejected("absent from the kernel trust set")

    # An embedded foreign signer must not override the certificate whose
    # membership was checked. This fixture would satisfy cms -verify without
    # -nointern, but must fail the production readiness check and kernel load.
    key = root / "foreign.key"
    pem = root / "foreign.pem"
    run("openssl", "req", "-new", "-x509", "-newkey", "ec", "-pkeyopt", "ec_paramgen_curve:P-256",
        "-nodes", "-subj", "/CN=foreign-module-fixture", "-keyout", str(key), "-out", str(pem))
    content = root / "content"
    content.write_bytes(unsigned)
    cms = run("openssl", "cms", "-sign", "-binary", "-noattr", "-in", str(content),
              "-signer", str(pem), "-inkey", str(key), "-outform", "DER")
    signature = root / "signature"
    signature.write_bytes(cms)
    trusted_pem = root / "trusted.pem"
    trusted_pem.write_bytes(run("openssl", "x509", "-inform", "DER", "-in", str(original_cert)))
    run("openssl", "cms", "-verify", "-binary", "-inform", "DER", "-in", str(signature),
        "-content", str(content), "-certfile", str(trusted_pem), "-noverify", "-out", "/dev/null")
    target.write_bytes(unsigned + cms + bytes([0, 0, 2, 0, 0, 0, 0, 0]) + struct.pack(">I", len(cms)) + marker)
    cert.write_bytes(certificate)
    rejected("signature does not match")
    result = subprocess.run(["insmod", str(target)], capture_output=True)
    assert result.returncode != 0 and b"Key was rejected" in result.stderr, result.stderr

print("modified/re-signed and embedded-foreign-certificate modules rejected")
