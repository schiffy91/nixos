{ settings, config, pkgs, lib, ... }:
let
  scale = toString settings.desktop.scalingFactor;
  # Steam strips %command% as a token for non-Steam shortcuts and always appends
  # the actual command at the end. Trailing args in launch options end up BEFORE
  # the command and break the shell invocation. The shim takes the command via
  # "$@" and appends the chrome flags after, where Battle.net's CEF picks them up.
  shim = pkgs.writeShellScript "battlenet-scale" ''
    exec "$@" --force-device-scale-factor=${scale} --high-dpi-support=1
  '';
  shimPath = "${config.home.homeDirectory}/.local/bin/battlenet-scale";
  launchOptions = "${shimPath} %command%";
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


    def ci_value(d, key):
        return next((v for k, v in d.items() if k.lower() == key), None)


    def ci_key(d, key, default):
        return next((k for k in d if k.lower() == key), default)


    def poke_log_pixels(prefix):
        user_reg = os.path.join(prefix, "user.reg")
        if not os.path.exists(user_reg):
            return
        with open(user_reg) as f:
            content = f.read()
        section = r'^\[Control Panel\\\\Desktop\] \d+$'
        header = re.search(section, content, re.M)
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
        if existing:
            body = line_re.sub(new_line, body)
        else:
            body = body.rstrip() + "\n" + new_line + "\n"
        tmp = user_reg + ".tmp"
        with open(tmp, "w") as f:
            f.write(content[:start] + body + content[end:])
        os.replace(tmp, user_reg)


    for path in glob.glob(STEAM + "/userdata/*/config/shortcuts.vdf"):
        with open(path, "rb") as f:
            config = vdf.binary_load(f)
        changed = False
        for entry in config.get("shortcuts", {}).values():
            name = ci_value(entry, "appname")
            if not name or name.lower() != NAME.lower():
                continue
            opt_key = ci_key(entry, "launchoptions", "LaunchOptions")
            if entry.get(opt_key) != OPTS:
                entry[opt_key] = OPTS
                changed = True
            exe = ci_value(entry, "exe") or ""
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
    install -Dm755 ${shim} "${shimPath}"
    if [ -d "${steamPath}/userdata" ]; then
      ${setBattlenetConfig}/bin/set-battlenet-config "${launchOptions}" "${steamPath}" "${scale}"
    fi
  '';
}
