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
  protonName = "GE-Proton10-34";
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
  setCompatTool = pkgs.writers.writePython3Bin "set-steam-compat-tool" {
    libraries = [ pkgs.python3Packages.vdf ];
  } ''
    import argparse
    import os

    import vdf

    parser = argparse.ArgumentParser()
    parser.add_argument("--steam-path", required=True)
    parser.add_argument("--tool-name", required=True)
    parser.add_argument("--app-id", default="0", help="0 = global default")
    parser.add_argument("--priority", default="250")
    args = parser.parse_args()

    path = args.steam_path + "/config/config.vdf"
    if not os.path.exists(path):
        raise SystemExit(0)

    with open(path) as f:
        cfg = vdf.load(f)

    valve = (
        cfg.setdefault("InstallConfigStore", {})
        .setdefault("Software", {})
        .setdefault("Valve", {})
        .setdefault("Steam", {})
    )
    mapping = valve.setdefault("CompatToolMapping", {})
    desired = {"name": args.tool_name, "config": "", "priority": args.priority}
    if mapping.get(args.app_id) == desired:
        raise SystemExit(0)
    mapping[args.app_id] = desired

    tmp = path + ".tmp"
    with open(tmp, "w") as f:
        vdf.dump(cfg, f, pretty=True)
    os.replace(tmp, path)
  '';
  # Per-app Steam config. Travels with the app — true wherever it's installed.
  # Apps not installed on a given host are harmless (the helpers no-op when no userdata exists).
  apps = {
    "221680" = {  # Rocksmith 2014 — ASIO + low-latency pipewire
      launchOptions = "LD_PRELOAD=/usr/lib32/libjack.so PIPEWIRE_LATENCY=${toString rsSampleSize}/${toString rsSampleRate} %command%";
    };
    "3240220" = {  # GTA V Enhanced
      # GE-Proton10-34 hangs the loader; 10-30 reaches Social Club then white-screens.
      # Proton Experimental + SteamDeck=1 is Valve's targeted fix for the launcher.
      compatTool = "proton_experimental";
      launchOptions = "SteamDeck=1 %command% ${chromiumDpi}";
    };
    "1174180" = {  # Red Dead Redemption 2 — Rockstar launcher is Chromium
      launchOptions = "%command% ${chromiumDpi}";
    };
    "1091500" = {  # Cyberpunk 2077 — REDlauncher is Chromium
      launchOptions = "%command% ${chromiumDpi}";
    };
  };
  perApp = appId: cfg:
    lib.optionalString (cfg ? compatTool) ''
      $runuser ${setCompatTool}/bin/set-steam-compat-tool \
        --steam-path "${steamPath}" \
        --tool-name "${cfg.compatTool}" \
        --app-id "${appId}"
    '' + lib.optionalString (cfg ? launchOptions) ''
      $runuser ${setLaunchOptions}/bin/set-steam-launch-options \
        --steam-path "${steamPath}" \
        --launch-options ${lib.escapeShellArg cfg.launchOptions} \
        --app-id "${appId}"
    '';
in {
  config = lib.mkMerge [
    {
      _module.args.steam = {
        inherit setLaunchOptions setCompatTool;
        proton.name = protonName;
      };
    }
    (lib.mkIf config.programs.steam.enable {
      system.activationScripts.steamApps = lib.stringAfter [ "users" ] ''
        if [ -d "${steamPath}/config" ]; then
          runuser="${pkgs.util-linux}/bin/runuser -u ${user} --"
          $runuser ${setCompatTool}/bin/set-steam-compat-tool \
            --steam-path "${steamPath}" \
            --tool-name "${protonName}"
          ${lib.concatStrings (lib.mapAttrsToList perApp apps)}
        fi
      '';
    })
  ];
}
