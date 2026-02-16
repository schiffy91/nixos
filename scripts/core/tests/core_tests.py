#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3 python3Packages.pytest
import os
import sys

sys.path.insert(
    0,
    os.path.join(
        os.path.dirname(
            os.path.abspath(__file__)),
        "..",
        ".."))

from core.shell import Shell  # noqa: E402


def main():
    sh = Shell()
    test_dir = os.path.dirname(os.path.abspath(__file__))
    scripts_dir = os.path.join(test_dir, "..", "..")
    ignore = os.path.join(test_dir, "vm_test.py")
    result = sh.run(
        f"cd '{scripts_dir}' && python3 -m pytest '{test_dir}' "
        f"'--ignore={ignore}' -v --tb=short",
        sudo=False, check=False,
    )
    sys.exit(result.returncode)


if __name__ == "__main__":
    main()
