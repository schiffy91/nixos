{ config, pkgs, lib, inputs, protonSge, winePrefixDpi, gameLaunch, assetto, ... }:
let
  user = config.settings.users.admin.username;
  home = config.settings.users.admin.homeDirectory;
  steamPath = "${home}/.local/share/Steam";
  protonSgeName = protonSge.name;  # patched Wayland+SNI build
  defaultLaunchPrefix = "${winePrefixDpi.launchPrefix} PROTON_ENABLE_HDR=1 DXVK_HDR=1";  # DPI into the prefix registry; no hdr_wsi layer (driver does HDR natively)
  games = import ./games.nix {
    inherit protonSgeName;
    inherit assetto;
    rsSampleSize = config.settings.apps.gaming.rocksmith.sampleSize;
    rsSampleRate = config.settings.apps.gaming.rocksmith.sampleRate;
  };
  appConfig = pkgs.writeText "steam-apps.json" (builtins.toJSON games);
  artwork = import ../artwork/package.nix { inherit pkgs inputs; };
  apps = config.settings.apps;
  shortcuts = pkgs.writeText "steam-shortcuts.json" (builtins.toJSON apps.gaming.steam.shortcuts);
  inherit (import ./lib.nix { inherit pkgs inputs; }) configureSteamApps;
  whenSteamIsClosed = import ./when-closed.nix { inherit pkgs user; ready = "${config.systemd.package}/bin/systemd-notify --ready"; };
  configureApps = lib.concatStringsSep " " [
    "${configureSteamApps}/bin/configure-steam-apps" "--steam-path" (lib.escapeShellArg steamPath) "--default-tool" (lib.escapeShellArg protonSgeName)
    "--default-launch-prefix" (lib.escapeShellArg defaultLaunchPrefix) "--launch-wrapper" (lib.escapeShellArg gameLaunch.protonPrefix) "--app-config" "${appConfig}"
  ];
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
    (lib.mkIf (config.settings.apps.gaming.steam.protonSge.enable) {
      # The compatibility tool of each game and its launch options, written while Steam is closed: before
      # the session at boot, at an update that changes what is declared (the unit changes, and a changed
      # unit that is active is started anew), and when Steam has just exited (it saves config.vdf as it
      # goes, and the path unit below then starts this again). With Steam open nothing is lost: it waits,
      # and `nixosctl pending` says so.
      systemd.services.steam-apps = {
        description = "Declared Steam compatibility tools and launch options";
        wantedBy = [ "multi-user.target" ];
        before = [ "display-manager.service" ];
        unitConfig.RequiresMountsFor = steamPath;
        unitConfig.ConditionPathIsDirectory = "${steamPath}/config";
        serviceConfig = {
          Type = "notify";
          NotifyAccess = "all";  # the readiness word comes from systemd-notify, a child of the script
          RemainAfterExit = true;  # done is still active, so that an update which changes it runs it again
          User = user;
          ExecStart = whenSteamIsClosed "steam-apps" configureApps;
        };
      };
      systemd.services.steam-apps-again = {
        description = "Steam saved its configuration: its declared tools and options are looked at again";
        serviceConfig = {
          Type = "oneshot";
          ExecStart = "${config.systemd.package}/bin/systemctl restart --no-block steam-apps.service";
        };
      };
      systemd.paths.steam-apps = {
        description = "Steam saved its configuration";
        wantedBy = [ "multi-user.target" ];
        pathConfig = {
          PathChanged = "${steamPath}/config/config.vdf";
          Unit = "steam-apps-again.service";
        };
      };
      environment.etc."${config.settings.apps.utils.nixosctl.pendingHooksDir}/steam-apps".source = pkgs.writeShellScript "steam-apps-pending" ''
        if ${pkgs.procps}/bin/pgrep -u ${user} -x steam > /dev/null && [ -d ${lib.escapeShellArg "${steamPath}/config"} ]; then
          if [ "$(${pkgs.coreutils}/bin/id -u)" = 0 ]; then asUser="${pkgs.util-linux}/bin/runuser -u ${user} --"; else asUser=; fi
          $asUser ${configureApps} --check > /dev/null 2>&1
          [ $? != 2 ] || echo "Steam is open, so the declared compatibility tools and launch options are not written yet: they are when Steam closes"
        fi
      '';
    })
    # For a controller's Desktop Layout, Steam types and moves the pointer
    # through the X server, and KWin asks the user whether it may. KWin keeps
    # the answer in kwinrc, which plasma-manager rewrites, so it is given here.
    (lib.mkIf (config.settings.apps.gaming.steam.enable && config.settings.desktop.enable && config.settings.users.admin.homeManager.enable) {
      home-manager.users.${user}.programs.plasma.configFile.kwinrc.Xwayland.XwaylandEisNoPromptApps = "steam";
    })
    (lib.mkIf (config.settings.apps.gaming.steam.enable && config.settings.apps.gaming.steam.shortcuts != {}) {
      environment.etc."${config.settings.apps.utils.nixosctl.pendingHooksDir}/steam-shortcuts".source = pkgs.writeShellScript "steam-shortcuts-pending" ''
        if ${pkgs.procps}/bin/pgrep -u ${user} -x steam > /dev/null; then
          echo "Steam is open, so its declared shortcuts were not rewritten: they are at the next boot, or with Steam closed by \`systemctl start steam-shortcuts\`"
        fi
      '';
      systemd.services.steam-shortcuts = {
        description = "Declared non-Steam shortcuts";
        wantedBy = [ "multi-user.target" ];
        before = [ "display-manager.service" ];
        unitConfig.RequiresMountsFor = steamPath;
        serviceConfig = {
          Type = "oneshot";
          User = user;
          ExecCondition = "${pkgs.bash}/bin/sh -c '! ${pkgs.procps}/bin/pgrep -u ${user} -x steam'";  # Steam rewrites shortcuts.vdf when it exits
          ExecStart = "${configureSteamApps}/bin/install-steam-shortcuts --steam-path ${steamPath} --shortcuts ${shortcuts} --machine ${config.networking.hostName}";
        };
      };
    })
  ];
}
