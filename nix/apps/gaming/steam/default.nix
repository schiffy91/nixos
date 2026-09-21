{ config, pkgs, lib, inputs, protonCustom, winePrefixDpi, assetto, ... }:
let
  user = config.settings.users.admin.username;
  home = "/home/${user}";
  steamPath = "${home}/.local/share/Steam";
  protonCustomName = protonCustom.name;  # patched Wayland+SNI build
  defaultLaunchPrefix = "${winePrefixDpi.launchPrefix} PROTON_ENABLE_WAYLAND=1 PROTON_ENABLE_HDR=1 DXVK_HDR=1";  # DPI into the prefix registry; no hdr_wsi layer (driver does HDR natively)
  games = import ./games.nix {
    inherit protonCustomName;
    inherit assetto;
    rsSampleSize = config.settings.apps.rocksmith.sampleSize;
    rsSampleRate = config.settings.apps.rocksmith.sampleRate;
  };
  appConfig = pkgs.writeText "steam-apps.json" (builtins.toJSON games);
  inherit (import ./lib.nix { inherit pkgs inputs; }) configureSteamApps;
in {
  config = lib.mkMerge [
    {
      _module.args.steam = { inherit configureSteamApps; };
    }
    (lib.mkIf (config.settings.apps.enable && config.settings.apps.gaming.enable && config.settings.apps.steam.enable && config.programs.steam.enable) {
      system.activationScripts.steamApps = lib.stringAfter [ "users" ] ''
        if ${pkgs.procps}/bin/pgrep -u ${user} -x steam >/dev/null; then
          echo "Steam is running; close it and rebuild to apply Steam settings."
        elif [ -d "${steamPath}/config" ]; then
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
