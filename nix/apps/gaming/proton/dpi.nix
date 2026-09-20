# One DPI for every Wine prefix, the way Windows does it: LogPixels in the
# prefix registry, derived from the primary output's scale. `set` writes it
# into named prefixes; `exec` writes it into the prefix a Steam launch is about
# to use ($STEAM_COMPAT_DATA_PATH/pfx or $WINEPREFIX) and execs the command.
{ config, pkgs, lib, ... }:
let
  user = config.settings.users.admin.username;
  primary = lib.findFirst (o: o.primary) null config.settings.desktop.outputs;
  scale = if primary == null then 1.0 else primary.scaleFactor;
  dpi = builtins.floor (96.0 * scale + 0.5);
  tool = pkgs.writers.writePython3Bin "wine-prefix-dpi" { flakeIgnore = [ "E501" ]; } ''
    import argparse
    import os
    import sys
    import time
    from pathlib import Path


    class RegFile:
        def __init__(self, path):
            self.path = path
            self.lines = path.read_text(encoding="utf-8", errors="surrogateescape").split("\n") if path.exists() else None

        def set_dword(self, section, name, value):
            if self.lines is None:
                return False
            header = "[" + section.replace("\\", "\\\\") + "]"
            line = f'"{name}"=dword:{value:08x}'
            start = next((i for i, l in enumerate(self.lines) if l.startswith(header + " ")), None)
            if start is None:
                while self.lines and self.lines[-1] == "":
                    self.lines.pop()
                self.lines += ["", f"{header} {int(time.time())}", line, ""]
                return True
            end = next((i for i in range(start + 1, len(self.lines)) if self.lines[i].startswith("[")), len(self.lines))
            for i in range(start + 1, end):
                if self.lines[i].startswith(f'"{name}"='):
                    if self.lines[i] == line:
                        return False
                    self.lines[i] = line
                    return True
            insert = start + 1
            while insert < end and self.lines[insert].startswith("#"):
                insert += 1
            self.lines.insert(insert, line)
            return True

        def save(self):
            tmp = self.path.with_name(self.path.name + ".tmp")
            tmp.write_text("\n".join(self.lines), encoding="utf-8", errors="surrogateescape")
            os.replace(tmp, self.path)


    class Prefix:
        values = (
            ("user.reg", "Control Panel\\Desktop", "LogPixels"),
            ("user.reg", "Software\\Wine\\Fonts", "LogPixels"),
            ("system.reg", "System\\ControlSet001\\Hardware Profiles\\Current\\Software\\Fonts", "LogPixels"),
        )

        def __init__(self, path):
            self.path = Path(path)

        def exists(self):
            return (self.path / "user.reg").exists()

        def set_dpi(self, dpi):
            changed = []
            for name, section, value in self.values:
                reg = RegFile(self.path / name)
                if reg.set_dword(section, value, dpi):
                    reg.save()
                    changed.append(f"{name}:{section}")
            return changed

        def dpi(self):
            reg = RegFile(self.path / "user.reg")
            for line in reg.lines or []:
                if line.startswith('"LogPixels"=dword:'):
                    return int(line.split(":")[1], 16)
            return None

        @classmethod
        def from_env(cls):
            compat = os.environ.get("STEAM_COMPAT_DATA_PATH")
            if compat:
                return cls(Path(compat) / "pfx")
            if os.environ.get("WINEPREFIX"):
                return cls(os.environ["WINEPREFIX"])
            return None


    def main():
        parser = argparse.ArgumentParser(prog="wine-prefix-dpi")
        sub = parser.add_subparsers(dest="command", required=True)
        p = sub.add_parser("set", help="write LogPixels into the given prefixes")
        p.add_argument("--dpi", type=int, required=True)
        p.add_argument("prefix", nargs="+")
        p = sub.add_parser("get", help="print the LogPixels of the given prefixes")
        p.add_argument("prefix", nargs="+")
        p = sub.add_parser("exec", help="set the launch prefix's DPI, then exec the command")
        p.add_argument("--dpi", type=int, required=True)
        p.add_argument("command", nargs=argparse.REMAINDER)
        args = parser.parse_args()
        if args.command == "set":
            for prefix in map(Prefix, args.prefix):
                if not prefix.exists():
                    print(f"{prefix.path}: no prefix yet", file=sys.stderr)
                    continue
                changed = prefix.set_dpi(args.dpi)
                print(f"{prefix.path}: {'updated ' + ', '.join(changed) if changed else 'already ' + str(args.dpi)}")
        elif args.command == "get":
            for prefix in map(Prefix, args.prefix):
                print(f"{prefix.path}: {prefix.dpi()}")
        else:
            command = args.command[1:] if args.command[:1] == ["--"] else args.command
            prefix = Prefix.from_env()
            if prefix is not None and prefix.exists():
                prefix.set_dpi(args.dpi)
            os.execvp("env", ["env", *command])


    main()
  '';
in lib.mkMerge [
  {
    _module.args.winePrefixDpi = {
      package = tool;
      inherit dpi;
      launchPrefix = "${tool}/bin/wine-prefix-dpi exec --dpi ${toString dpi} --";  # goes in front of Steam launch options
    };
  }
  (lib.mkIf (config.settings.apps.enable && config.settings.apps.gaming.enable) {
    users.users.${user}.packages = [ tool ];
  })
]
