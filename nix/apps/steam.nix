{ config, pkgs, lib, ... }:
let
  user = config.settings.user.admin.username;
  home = "/home/${user}";
  steamPath = "${home}/.local/share/Steam";
  primary = lib.findFirst (o: o.primary) null config.settings.desktop.outputs;
  scale = if primary == null then 1.0 else primary.scaleFactor;
  chromiumDpi = "--force-device-scale-factor=${toString scale} --high-dpi-support=1";
  rsSampleSize = config.settings.rocksmith.sampleSize;
  rsSampleRate = config.settings.rocksmith.sampleRate;
  protonName = "GE-Proton10-34";       # upstream binary (fallback / X11)
  protonCustomName = "proton-custom-GE-Proton10-34";  # patched Wayland+SNI build
  defaultLaunchPrefix = "PROTON_ENABLE_WAYLAND=1 PROTON_ENABLE_HDR=1 DXVK_HDR=1 ENABLE_HDR_WSI=1";
  appConfig = pkgs.writeText "steam-apps.json" (builtins.toJSON apps);
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

    with open(args.app_config) as f:
        app_config = json.load(f)


    def desired_entry(tool):
        return {"name": tool, "config": "", "priority": "250"}


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

        desired = desired_entry(args.default_tool)
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
  # Per-app Steam config. Travels with the app — true wherever it's installed.
  # Installed apps not listed here inherit proton-custom + the default Steam Play env.
  apps = {
    "221680" = {  # Rocksmith 2014 — ASIO + low-latency pipewire
      compatTool = protonCustomName;
      launchOptions = "LD_PRELOAD=/usr/lib32/libjack.so PIPEWIRE_LATENCY=${toString rsSampleSize}/${toString rsSampleRate} %command%";
    };
    "3240220" = {  # GTA V Enhanced
      # GE-Proton10-34 hangs the loader; 10-30 reaches Social Club then white-screens.
      # Proton Experimental + SteamDeck=1 is Valve's targeted fix for the launcher.
      compatTool = "proton_experimental";
      launchPrefix = "SteamDeck=1";
      launchSuffix = chromiumDpi;
    };
    "1174180" = {  # Red Dead Redemption 2 — Rockstar launcher is Chromium
      launchSuffix = chromiumDpi;
    };
    "1091500" = {  # Cyberpunk 2077 — REDlauncher is Chromium
      launchSuffix = chromiumDpi;
    };
  };
in {
  config = lib.mkMerge [
    {
      _module.args.steam = {
        inherit configureSteamApps;
        proton.name = protonName;
        proton.customName = protonCustomName;
      };
    }
    (lib.mkIf (config.settings.apps.enable && config.settings.apps.steam.enable && config.programs.steam.enable) {
      system.activationScripts.steamApps = lib.stringAfter [ "users" ] ''
        if [ -d "${steamPath}/config" ]; then
          runuser="${pkgs.util-linux}/bin/runuser -u ${user} --"
          $runuser ${configureSteamApps}/bin/configure-steam-apps \
            --steam-path "${steamPath}" \
            --default-tool "${protonCustomName}" \
            --default-launch-prefix ${lib.escapeShellArg defaultLaunchPrefix} \
            --app-config ${appConfig}
        fi
      '';
    })
  ];
}
