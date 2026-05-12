{ pkgs }:
let
  setLaunchOptions = pkgs.writers.writePython3Bin "set-steam-launch-options" {
    libraries = [ pkgs.python3Packages.vdf ];
  } ''
    import argparse
    import glob
    import os

    import vdf

    parser = argparse.ArgumentParser()
    parser.add_argument("--steam-path", required=True)
    parser.add_argument("--launch-options", required=True)
    target = parser.add_mutually_exclusive_group(required=True)
    target.add_argument(
        "--app-id", help="Steam app ID for purchased games (localconfig.vdf)"
    )
    target.add_argument(
        "--name-match",
        help="Substring of AppName for non-Steam shortcuts (shortcuts.vdf)",
    )
    args = parser.parse_args()


    def set_for_app_id():
        pattern = args.steam_path + "/userdata/*/config/localconfig.vdf"
        for path in glob.glob(pattern):
            with open(path) as f:
                cfg = vdf.load(f)
            steam = (
                cfg.setdefault("UserLocalConfigStore", {})
                .setdefault("Software", {})
                .setdefault("Valve", {})
                .setdefault("Steam", {})
            )
            apps = steam.get("apps") or steam.setdefault("Apps", {})
            app = apps.setdefault(args.app_id, {})
            if app.get("LaunchOptions") == args.launch_options:
                continue
            app["LaunchOptions"] = args.launch_options
            with open(path, "w") as f:
                vdf.dump(cfg, f, pretty=True)


    def set_for_non_steam():
        match = args.name_match.lower()
        pattern = args.steam_path + "/userdata/*/config/shortcuts.vdf"
        for path in glob.glob(pattern):
            with open(path, "rb") as f:
                cfg = vdf.binary_load(f)
            changed = False
            for entry in cfg.get("shortcuts", {}).values():
                name = next(
                    (v for k, v in entry.items() if k.lower() == "appname"),
                    None,
                )
                if not name or match not in name.lower():
                    continue
                key = next(
                    (k for k in entry if k.lower() == "launchoptions"),
                    "LaunchOptions",
                )
                if entry.get(key) != args.launch_options:
                    entry[key] = args.launch_options
                    changed = True
            if changed:
                tmp = path + ".tmp"
                with open(tmp, "wb") as f:
                    vdf.binary_dump(cfg, f)
                os.replace(tmp, path)


    if args.app_id:
        set_for_app_id()
    else:
        set_for_non_steam()
  '';
in {
  inherit setLaunchOptions;
}
