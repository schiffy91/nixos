{ settings, config, pkgs, lib, ... }:
let
  scale = toString settings.desktop.scalingFactor;
  launchOptions = "%command% --force-device-scale-factor=${scale} --high-dpi-support=1";
  setBattlenetConfig = pkgs.writers.writePython3Bin "set-battlenet-config" {
    libraries = [ pkgs.python3Packages.vdf ];
  } ''
    import glob
    import os
    import re
    import sys
    import vdf

    NAME = "Battle.net"
    OPTS, STEAM, SCALE = sys.argv[1], sys.argv[2], float(sys.argv[3])
    LOG_PIXELS = round(96 * SCALE)

    def poke_log_pixels(prefix):
        user_reg = os.path.join(prefix, "user.reg")
        if not os.path.exists(user_reg):
            return
        with open(user_reg) as f:
            content = f.read()
        header = re.search(r'^\[Control Panel\\\\Desktop\] \d+$', content, re.M)
        if not header:
            return
        start = header.end()
        nxt = re.search(r'\n\[', content[start:])
        end = start + nxt.start() if nxt else len(content)
        body = content[start:end]
        new_line = f'"LogPixels"=dword:{LOG_PIXELS:08x}'
        line_re = re.compile(r'^"LogPixels"=dword:[0-9a-f]+$', re.M)
        existing = line_re.search(body)
        if existing and existing.group(0) == new_line:
            return
        body = line_re.sub(new_line, body) if existing else body.rstrip() + '\n' + new_line + '\n'
        tmp = user_reg + ".tmp"
        with open(tmp, "w") as f:
            f.write(content[:start] + body + content[end:])
        os.replace(tmp, user_reg)

    for path in glob.glob(STEAM + "/userdata/*/config/shortcuts.vdf"):
        with open(path, "rb") as f:
            config = vdf.binary_load(f)
        changed = False
        for entry in config.get("shortcuts", {}).values():
            name = next((v for k, v in entry.items() if k.lower() == "appname"), None)
            if not name or name.lower() != NAME.lower():
                continue
            opt_key = next((k for k in entry if k.lower() == "launchoptions"), "LaunchOptions")
            if entry.get(opt_key) != OPTS:
                entry[opt_key] = OPTS
                changed = True
            exe = next((v for k, v in entry.items() if k.lower() == "exe"), "")
            m = re.search(r'(/.+/compatdata/\d+/pfx)/', exe)
            if m:
                poke_log_pixels(m.group(1))
        if changed:
            tmp = path + ".tmp"
            with open(tmp, "wb") as f:
                vdf.binary_dump(config, f)
            os.replace(tmp, path)
  '';
  steamPath = "${config.home.homeDirectory}/.local/share/Steam";
in {
  home.activation.battlenet = lib.hm.dag.entryAfter [ "writeBoundary" ] ''
    if [ -d "${steamPath}/userdata" ]; then
      ${setBattlenetConfig}/bin/set-battlenet-config "${launchOptions}" "${steamPath}" "${scale}"
    fi
  '';
}
