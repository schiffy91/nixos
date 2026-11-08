{ config, pkgs, lib, inputs, protonCustom, winePrefixDpi, gameLaunch, assetto, ... }:
let
  user = config.settings.users.admin.username;
  home = config.settings.users.admin.homeDirectory;
  steamPath = "${home}/.local/share/Steam";
  protonCustomName = protonCustom.name;  # patched Wayland+SNI build
  defaultLaunchPrefix = "${winePrefixDpi.launchPrefix} PROTON_ENABLE_HDR=1 DXVK_HDR=1";  # DPI into the prefix registry; no hdr_wsi layer (driver does HDR natively)
  games = import ./games.nix {
    inherit protonCustomName;
    inherit assetto;
    rsSampleSize = config.settings.apps.gaming.rocksmith.sampleSize;
    rsSampleRate = config.settings.apps.gaming.rocksmith.sampleRate;
  };
  appConfig = pkgs.writeText "steam-apps.json" (builtins.toJSON games);
  artwork = import ../artwork/package.nix { inherit pkgs inputs; };
  apps = config.settings.apps;
  shortcuts = pkgs.writeText "steam-shortcuts.json" (builtins.toJSON apps.gaming.steam.shortcuts);
  inherit (import ./lib.nix { inherit pkgs inputs; }) configureSteamApps;
in {
  config = lib.mkMerge [
    (lib.mkIf (config.settings.apps.gaming.steam.enable) {
      environment.systemPackages = [ pkgs.sgdboop ];
      settings.apps.gaming.steam.shortcuts = lib.mapAttrs (_: shortcut: lib.mkDefault shortcut) (
        lib.optionalAttrs apps.gaming.battlenet.enable {
          "Battle.net" = { exe = "/etc/profiles/per-user/${user}/bin/battlenet"; } // artwork.battlenet;
        } // lib.optionalAttrs apps.gaming.semu.enable {
          Semu = { exe = "/run/current-system/sw/bin/semu-es-de"; } // artwork.semu;
        } // lib.optionalAttrs apps.utils.chrome.enable {
          Chrome = { exe = "/run/current-system/sw/bin/google-chrome-stable"; } // artwork.chrome;
        } // lib.optionalAttrs apps.gaming.moonlight.enable {
          Moonlight = { exe = "/run/current-system/sw/bin/moonlight"; } // artwork.moonlight;
        } // lib.optionalAttrs apps.gaming.chiaki.enable {
          chiaki-ng = { exe = "/run/current-system/sw/bin/chiaki"; } // artwork.chiaki;
        });
      settings.disk.immutability.persist.paths = [ "${home}/.local/share/Steam" "${home}/.steam" ];
      programs.steam = {
        enable = true;
        # Jovian supplies the Deck's Steam package and session integration.
        package = lib.mkDefault (pkgs.steam.override {
          extraLibraries = pkgs': [ pkgs'.pipewire.jack ] ++ config.settings.apps.gaming.steam.extraLibraries;
          extraEnv = {
            PROTON_ENABLE_WAYLAND = "1";
            PROTON_ENABLE_HDR = "1";
          } // config.settings.apps.gaming.steam.extraEnv;
        });
      };
    })
    {
      _module.args.steam = { inherit configureSteamApps; };
    }
    (lib.mkIf (config.settings.apps.gaming.steam.protonCustom.enable) {
      system.activationScripts.steamApps = lib.stringAfter [ "users" ] ''
        if ${pkgs.procps}/bin/pgrep -u ${user} -x steam >/dev/null; then
          echo "Steam is running; close it and rebuild to apply Steam settings."
        elif [ -d "${steamPath}/config" ]; then
          runuser="${pkgs.util-linux}/bin/runuser -u ${user} --"
          $runuser ${configureSteamApps}/bin/configure-steam-apps \
            --steam-path "${steamPath}" \
            --default-tool "${protonCustomName}" \
            --default-launch-prefix ${lib.escapeShellArg defaultLaunchPrefix} \
            --launch-wrapper ${lib.escapeShellArg gameLaunch.protonPrefix} \
            --app-config ${appConfig}
        fi
      '';
    })
    (lib.mkIf (config.settings.apps.gaming.steam.enable && config.settings.apps.gaming.steam.shortcuts != {}) {
      systemd.services.steam-shortcuts = {
        description = "Declared non-Steam shortcuts";
        wantedBy = [ "multi-user.target" ];
        before = [ "display-manager.service" ];
        unitConfig.RequiresMountsFor = steamPath;
        serviceConfig = {
          Type = "oneshot";
          User = user;
          ExecCondition = "${pkgs.bash}/bin/sh -c '! ${pkgs.procps}/bin/pgrep -u ${user} -x steam'";  # Steam rewrites shortcuts.vdf when it exits
          ExecStart = "${configureSteamApps}/bin/install-steam-shortcuts --steam-path ${steamPath} --shortcuts ${shortcuts}";
        };
      };
    })
  ];
}
