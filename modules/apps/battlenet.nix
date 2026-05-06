{ config, pkgs, lib, ... }:
let
  launchOptions =
    "PROTON_ENABLE_WAYLAND=1 PROTON_ENABLE_HDR=1 DXVK_HDR=1 %command%";
  setLaunchOptions = pkgs.writers.writePython3Bin "set-battlenet-launch-options" {
    libraries = [ pkgs.python3Packages.vdf ];
  } ''
    import glob
    import os
    import sys

    import vdf

    OPTS, STEAM = sys.argv[1], sys.argv[2]
    for path in glob.glob(STEAM + "/userdata/*/config/shortcuts.vdf"):
        with open(path, "rb") as f:
            cfg = vdf.binary_load(f)
        changed = False
        for entry in cfg.get("shortcuts", {}).values():
            name = next(
                (v for k, v in entry.items() if k.lower() == "appname"), None
            )
            if not name or "battle.net" not in name.lower():
                continue
            key = next(
                (k for k in entry if k.lower() == "launchoptions"),
                "LaunchOptions",
            )
            if entry.get(key) != OPTS:
                entry[key] = OPTS
                changed = True
        if changed:
            tmp = path + ".tmp"
            with open(tmp, "wb") as f:
                vdf.binary_dump(cfg, f)
            os.replace(tmp, path)
  '';
  steamPath = "${config.home.homeDirectory}/.local/share/Steam";
in {
  home.activation.battlenet = lib.hm.dag.entryAfter [ "writeBoundary" ] ''
    if [ -d "${steamPath}/userdata" ]; then
      ${setLaunchOptions}/bin/set-battlenet-launch-options "${launchOptions}" "${steamPath}"
    fi
  '';
}
