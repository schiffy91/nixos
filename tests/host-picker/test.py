"""Exercise the real Nix registry, subprocess output and interactive host picker."""
import json
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
stdlib = root / "build/stdlib"
registry = subprocess.run(
    ["nix", "--extra-experimental-features", "nix-command flakes", "eval", "--json", f"{root}#lib.hosts"],
    check=True, capture_output=True, text=True,
)
hosts = json.loads(registry.stdout)
assert hosts and all(host["system"].endswith("-linux") for host in hosts)
with tempfile.TemporaryDirectory(prefix="host-picker-") as temporary:
    directory = Path(temporary)
    source = directory / "picker.c"
    binary = directory / "picker"
    subprocess.run([
        "nix", "run", "--no-warn-dirty", "--inputs-from", str(root), "btrc#btrcpy", "--",
        "--no-stdlib", "--strict-imports", "--stdlib", str(stdlib),
        str(root / "tests/host-picker/Driver.btrc"), "-o", str(source),
    ], check=True)
    subprocess.run([
        "cc", "-std=c11", "-pedantic", "-I", str(stdlib), str(source),
        str(root / "build/libbtrc.a"), "-lm", "-lpthread", "-lutil", "-o", str(binary),
    ], check=True)
    for index, host in enumerate(hosts, start=1):
        result = subprocess.run([str(binary), str(root / "config/hosts")],
                                input=f"{index}\n", capture_output=True, text=True, timeout=30)
        expected = root / "config/hosts" / host["path"]
        assert result.returncode == 0, result.stdout + result.stderr
        assert f"SELECTED={expected}" in result.stdout, result.stdout + result.stderr
        assert expected.is_file()
    retry = subprocess.run([str(binary), str(root / "config/hosts")],
                           input="999\n1\n", capture_output=True, text=True, timeout=30)
    assert retry.returncode == 0 and f"SELECTED={root / 'config/hosts' / hosts[0]['path']}" in retry.stdout
    eof = subprocess.run([str(binary), str(root / "config/hosts")],
                         input="", capture_output=True, text=True, timeout=30)
    assert eof.returncode != 0 and "No host chosen" in eof.stderr, eof.stdout + eof.stderr
    # Nix diagnostics must remain separate from JSON even in a clean CI checkout.
    fixture = directory / "diagnostic-flake"
    host = fixture / "config/hosts/x86_64/FIXTURE/default.nix"
    host.parent.mkdir(parents=True)
    host.write_text("{}\n")
    (fixture / "flake.nix").write_text('''{
      outputs = _: {
        lib.hosts = builtins.trace "host-picker diagnostic" [{
          name = "FIXTURE"; system = "x86_64-linux";
          outputName = "FIXTURE"; path = "x86_64/FIXTURE/default.nix";
        }];
      };
    }\n''')
    diagnostic = subprocess.run([str(binary), str(fixture / "config/hosts")],
                                input="1\n", capture_output=True, text=True, timeout=30)
    assert diagnostic.returncode == 0, diagnostic.stdout + diagnostic.stderr
    assert "host-picker diagnostic" in diagnostic.stderr, diagnostic.stderr
    assert f"SELECTED={host}" in diagnostic.stdout, diagnostic.stdout
print("PASS: real registry selects every Linux host, tolerates Nix diagnostics and bounds invalid/EOF input")
