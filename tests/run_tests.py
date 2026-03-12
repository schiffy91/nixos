#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3 python3Packages.pytest qemu libarchive openssh OVMF
import subprocess, sys

SUITES = {
    "unit":        "tests/unit_tests/",
    "integration": "tests/integration_tests/",
    "vm":          "tests/functional_tests/vm_test.py",
    "all":         "tests/",
}

def main():
    suite = sys.argv[1] if len(sys.argv) > 1 else "all"
    if suite not in SUITES:
        print(f"Usage: run_tests.py [{' | '.join(SUITES)}]")
        sys.exit(1)
    args = ["python", "-m", "pytest", SUITES[suite], "-v", "-x"]
    if suite in ("vm", "all"): args.append("-s")
    args += sys.argv[2:]
    sys.exit(subprocess.run(args, check=False).returncode)

if __name__ == "__main__": main()
