"""Verify the BTRC/libcrypto boundary using freshly generated fixture keys."""
from pathlib import Path
import signal
import subprocess
import sys
import tempfile


def run(*args):
    return subprocess.run(args, check=True, capture_output=True, timeout=15).stdout


with tempfile.TemporaryDirectory(prefix="secure-boot-crypto-") as temporary:
    root = Path(temporary)
    for name in ("first", "second"):
        run("openssl", "req", "-new", "-x509", "-newkey", "ec", "-pkeyopt", "ec_paramgen_curve:P-256",
            "-nodes", "-subj", f"/CN={name}", "-keyout", str(root / f"{name}.key"), "-out", str(root / f"{name}.pem"))
    key = root / "first.key"
    certificate = root / "first.pem"
    expected = run("openssl", "x509", "-in", str(certificate), "-fingerprint", "-sha256", "-noout").strip().split(b"=", 1)[1]
    assert run(sys.argv[1], str(key), str(certificate)).strip() == expected
    (root / "key.der").write_bytes(run("openssl", "pkey", "-in", str(key), "-outform", "DER"))
    (root / "certificate.der").write_bytes(run("openssl", "x509", "-in", str(certificate), "-outform", "DER"))
    assert run(sys.argv[1], str(root / "key.der"), str(root / "certificate.der")).strip() == expected
    (root / "invalid.key").write_bytes(b"invalid\0private-key")
    run("openssl", "pkey", "-in", str(key), "-aes256", "-passout", "pass:fixture", "-out", str(root / "encrypted.key"))
    for bad_key, cert in ((root / "second.key", certificate), (root / "invalid.key", certificate),
                          (root / "encrypted.key", certificate), (key, root / "invalid.key")):
        result = subprocess.run([sys.argv[1], str(bad_key), str(cert)], capture_output=True, timeout=5)
        assert result.returncode, (bad_key, cert)
        assert b"PRIVATE KEY" not in result.stderr and b"BEGIN" not in result.stderr
    def inherited_mask():
        signal.signal(signal.SIGALRM, signal.SIG_IGN)
        signal.pthread_sigmask(signal.SIG_BLOCK, {signal.SIGALRM})

    expired = subprocess.run([sys.argv[1], "--deadline"], capture_output=True, timeout=3, preexec_fn=inherited_mask)
    assert expired.returncode == -signal.SIGALRM, expired
    print("PASS: PEM/DER matching, canonical fingerprint, mismatched/corrupt/encrypted keys, no prompting or key output")
