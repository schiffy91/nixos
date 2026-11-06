#!/usr/bin/env nix-shell
#!nix-shell -i python3 -p python3
"""Build this host's share of .github/ci/targets.nix, sign it, and add it to a flat-file Nix
binary cache the other machines substitute from over HTTP. No account anywhere.

  .github/ci/publish.py [--all] [--dir DIR] [--key FILE] [ATTR...]

--all uses the `local` list instead of this system's CI attributes. The secret key is
generated on first use, stays on this machine, and its public half is printed: that is
what goes into settings.nix.trustedPublicKeys on every other host.
"""
import argparse, json, os, pathlib, subprocess, sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
TARGETS = pathlib.Path(__file__).resolve().parent / "targets.nix"


class Cache:
    @classmethod
    def evaluate(cls, apply, raw=True):
        flags = ["--raw"] if raw else ["--json"]
        out = subprocess.run(["nix", "eval", *flags, "--file", str(TARGETS), "--apply", apply],
                             capture_output=True, text=True, check=True).stdout
        return out if raw else json.loads(out)

    @classmethod
    def system(cls):
        return subprocess.run(["nix", "eval", "--raw", "--impure", "--expr", "builtins.currentSystem"],
                              capture_output=True, text=True, check=True).stdout

    @classmethod
    def attrs(cls, system, every):
        if every:
            return cls.evaluate("c: builtins.concatStringsSep \" \" c.local").split()
        apply = ('c: let m = builtins.filter (s: s.system == "%s") c.systems; in '
                 'if m == [] then "" else builtins.concatStringsSep " " (builtins.head m).attrs' % system)
        return cls.evaluate(apply).split()

    @classmethod
    def key(cls, path, name):
        path = pathlib.Path(path).expanduser()
        if not path.exists():
            path.parent.mkdir(parents=True, exist_ok=True)
            secret = subprocess.run(["nix", "key", "generate-secret", "--key-name", name],
                                    capture_output=True, text=True, check=True).stdout
            path.write_text(secret)
            path.chmod(0o600)
            print(f"publish: generated {path} — keep a copy", file=sys.stderr)
        public = subprocess.run(["nix", "key", "convert-secret-to-public"], input=path.read_text(),
                                capture_output=True, text=True, check=True).stdout.strip()
        return path, public

    @classmethod
    def run(cls, arguments):
        system = cls.system()
        name = cls.evaluate("c: c.cache.name")
        declared = cls.evaluate("c: c.cache.publicKey")
        path, public = cls.key(arguments.key, name)
        if declared and declared != public:
            print(f"publish: this key is {public} but .github/ci/targets.nix declares {declared}", file=sys.stderr)
        wanted = arguments.attrs or cls.attrs(system, arguments.all)
        if not wanted:
            sys.exit(f"publish: .github/ci/targets.nix names nothing for {system}")
        directory = pathlib.Path(arguments.dir).expanduser().resolve()
        directory.mkdir(parents=True, exist_ok=True)
        for attr in wanted:
            print(f"publish: {system}#{attr}", file=sys.stderr)
            built = subprocess.run(["nix", "build", "--no-link", "--print-out-paths", f"{ROOT}#{attr}"],
                                   capture_output=True, text=True, check=True).stdout.split()
            subprocess.run(["nix", "store", "sign", "--key-file", str(path), "--recursive", *built], check=True)
            subprocess.run(["nix", "copy", "--to", f"file://{directory}?compression=zstd", *built], check=True)
        served = len(list(directory.glob("*.narinfo")))
        print(f"\npublish: {directory} holds {served} paths. Serve it (make cache-serve) and on every other host:\n"
              f"  settings.nix.substituters = [ \"http://{os.uname().nodename}.local:8080\" ];\n"
              f"  settings.nix.trustedPublicKeys = [ \"{public}\" ];", file=sys.stderr)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("attrs", nargs="*", help="flake attributes to build instead of the list in .github/ci/targets.nix")
    parser.add_argument("--all", action="store_true", help="build the `local` list: everything a host activates")
    parser.add_argument("--dir", default="~/.cache/nix-cache", help="the cache directory, shared with the other repositories on this host")
    parser.add_argument("--key", default="~/.config/nix/nixos-cache-1.key", help="the signing key")
    Cache.run(parser.parse_args())
