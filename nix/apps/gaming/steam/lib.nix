# Reconciles Steam's own VDFs with the declared per-app config.
{ pkgs }:
{
  configureSteamApps = pkgs.writers.writePython3Bin "configure-steam-apps" {
    libraries = [ pkgs.python3Packages.vdf ];
  } ''
    import argparse
    import glob
    import json
    import os
    from pathlib import Path

    import vdf

    parser = argparse.ArgumentParser()
    parser.add_argument("--steam-path", required=True)
    parser.add_argument("--default-tool", required=True)
    parser.add_argument("--default-launch-prefix", default="")
    parser.add_argument("--default-launch-suffix", default="")
    parser.add_argument("--app-config", required=True)
    args = parser.parse_args()

    skip_names = {"Steamworks Common Redistributables"}
    skip_prefixes = ("Proton ", "Steam Linux Runtime")
    wildcard_priority = "75"  # 250 would hijack native tools like SLR 4.0

    with open(args.app_config) as f:
        app_config = json.load(f)


    def desired_entry(tool, priority="250"):
        return {"name": tool, "config": "", "priority": priority}


    def words(*parts):
        return " ".join(part.strip() for part in parts if part and part.strip())


    def text_vdf(path):
        with open(path) as f:
            return vdf.load(f)


    def write_text_vdf(path, cfg):
        tmp = str(path) + ".tmp"
        with open(tmp, "w") as f:
            vdf.dump(cfg, f, pretty=True)
        os.replace(tmp, path)


    def steam_apps(cfg):
        steam = (
            cfg.setdefault("UserLocalConfigStore", {})
            .setdefault("Software", {})
            .setdefault("Valve", {})
            .setdefault("Steam", {})
        )
        return steam.get("apps") or steam.setdefault("Apps", {})


    def library_paths():
        path = Path(args.steam_path) / "steamapps" / "libraryfolders.vdf"
        if not path.exists():
            return [Path(args.steam_path)]

        cfg = text_vdf(path)
        paths = []
        for entry in cfg.get("libraryfolders", {}).values():
            if isinstance(entry, dict) and entry.get("path"):
                paths.append(Path(entry["path"]))
        return paths or [Path(args.steam_path)]


    def installed_games():
        manifests = {}
        for library in library_paths():
            for path in (library / "steamapps").glob("appmanifest_*.acf"):
                app_id = path.stem.removeprefix("appmanifest_")
                manifests.setdefault(app_id, path)

        def by_app_id(item):
            return int(item[0])

        for app_id, path in sorted(manifests.items(), key=by_app_id):
            app = text_vdf(path).get("AppState", {})
            name = app.get("name", "")
            if name in skip_names or name.startswith(skip_prefixes):
                continue
            yield app_id


    def launch_options(app_id):
        cfg = app_config.get(app_id, {})
        if "launchOptions" in cfg:
            return cfg["launchOptions"]

        inherit_default = cfg.get("inheritDefaultLaunchOptions", True)
        prefix = words(
            cfg.get("launchPrefix", ""),
            args.default_launch_prefix if inherit_default else "",
        )
        suffix = words(
            args.default_launch_suffix if inherit_default else "",
            cfg.get("launchSuffix", ""),
        )
        return words(prefix, "%command%", suffix)


    app_ids = list(installed_games())

    path = Path(args.steam_path) / "config" / "config.vdf"
    if path.exists():
        cfg = text_vdf(path)
        valve = (
            cfg.setdefault("InstallConfigStore", {})
            .setdefault("Software", {})
            .setdefault("Valve", {})
            .setdefault("Steam", {})
        )
        mapping = valve.setdefault("CompatToolMapping", {})
        changed = False

        desired = desired_entry(args.default_tool, wildcard_priority)
        if mapping.get("0") != desired:
            mapping["0"] = desired
            changed = True

        for app_id in app_ids:
            tool = app_config.get(app_id, {}).get("compatTool", args.default_tool)
            if tool is None:
                continue
            desired = desired_entry(tool)
            if mapping.get(app_id) != desired:
                mapping[app_id] = desired
                changed = True

        if changed:
            write_text_vdf(path, cfg)

    for path in glob.glob(args.steam_path + "/userdata/*/config/localconfig.vdf"):
        cfg = text_vdf(path)
        apps = steam_apps(cfg)
        changed = False

        for app_id in app_ids:
            app = apps.setdefault(app_id, {})
            desired = launch_options(app_id)
            if app.get("LaunchOptions") != desired:
                app["LaunchOptions"] = desired
                changed = True

        if changed:
            write_text_vdf(path, cfg)
  '';
}
