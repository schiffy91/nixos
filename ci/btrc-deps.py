"""btrc-deps: the sources each BTRC program is made from, as make variables.

  btrc-deps.py NAME=ENTRY.btrc ...   prints `DEPS_NAME := file ...` for each program

A program is made from its entry file, every file that imports reach from it, and the manifest and
headers of each package those files are in. make rebuilds a program when one of these changes
instead of when anything does. Standard library imports are left out: they change with flake.lock."""

import re
import sys
import tomllib
from pathlib import Path

IMPORT = re.compile(r"^\s*import\s+([^\s;{]+)", re.MULTILINE)


class Packages:
    roots = {}
    manifests = {}

    @classmethod
    def root(cls, file):
        directory = file.parent
        if directory not in cls.roots:
            found = next((parent for parent in [directory, *directory.parents] if (parent / "btrc.toml").is_file()), None)
            cls.roots[directory] = found
        return cls.roots[directory]

    @classmethod
    def manifest(cls, root):
        if root not in cls.manifests:
            cls.manifests[root] = tomllib.loads((root / "btrc.toml").read_text())
        return cls.manifests[root]

    @classmethod
    def module(cls, file, name):
        """The file a `package.Module.Path` import names, or None for the standard library."""
        root = cls.root(file)
        if root is None:
            return None
        package, _, path = name.partition(".")
        manifest = cls.manifest(root)
        if package == manifest.get("package", {}).get("name"):
            base = root
        else:
            dependency = manifest.get("dependencies", {}).get(package)
            if not isinstance(dependency, dict) or "path" not in dependency:
                return None
            base = (root / dependency["path"]).resolve()
        candidate = base / (path.replace(".", "/") + ".btrc")
        return candidate if candidate.is_file() else None


class Program:
    @classmethod
    def imports(cls, file):
        for name in IMPORT.findall(file.read_text()):
            if name.startswith("."):
                yield (file.parent / name).resolve()
            elif not name.startswith("Library."):
                target = Packages.module(file, name)
                if target is not None:
                    yield target

    @classmethod
    def sources(cls, entry):
        seen, pending = set(), [entry.resolve()]
        while pending:
            file = pending.pop()
            if file in seen or not file.is_file():
                continue
            seen.add(file)
            pending.extend(cls.imports(file))
        for root in {Packages.root(file) for file in seen} - {None}:
            seen.add(root / "btrc.toml")  # not btrc.lock: the compiler rewrites it from the manifest on every build
            seen.update(header for header in root.rglob("*.h") if Packages.root(header) == root)  # a package nested in this one has its own
        return sorted(seen)


def main():
    if len(sys.argv) < 2 or sys.argv[1] in ("-h", "--help"):
        print(__doc__)
        sys.exit(0 if len(sys.argv) > 1 else 1)
    here = Path.cwd()
    for argument in sys.argv[1:]:
        name, _, entry = argument.partition("=")
        # A package outside the repository (btrc's testkit, linked from the store) changes only with flake.lock.
        files = " ".join(str(path.relative_to(here)).replace(" ", "\\ ") for path in Program.sources(Path(entry)) if path.is_relative_to(here))
        print(f"DEPS_{name} := {files}")


main()
