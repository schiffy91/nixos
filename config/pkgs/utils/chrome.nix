{ config, lib, pkgs, inputs, ... }:
let
  chrome = config.settings.apps.utils.chrome;
  enabled = chrome.enable;
  apps = lib.filterAttrs (_: app: app.enable) chrome.webApps;
  flags = chrome.flags ++ lib.optional (chrome.renderNode != null) "--render-node-override=${chrome.renderNode}";
  launchers = import ./chrome-launchers/package.nix { inherit pkgs inputs; };
  declaration = pkgs.writeText "chrome-apps.json" (builtins.toJSON {
    inherit (chrome) profile;
    apps = lib.mapAttrs (_: app: app.name) apps;
  });
in lib.mkMerge [
  (lib.mkIf enabled {
    assertions = [
      {
        assertion = lib.all (key: builtins.match "[a-zA-Z0-9_-]+" key != null) (builtins.attrNames apps);
        message = "Chrome web-app keys must contain only letters, digits, underscores and hyphens.";
      }
      {
        assertion = let names = map (app: app.name) (builtins.attrValues apps);
          in builtins.length names == builtins.length (lib.unique names);
        message = "Chrome web-app names must be unique to resolve their native launchers.";
      }
    ];
    settings.disk.immutability.persist.paths = [ "${config.settings.users.admin.homeDirectory}/.config/google-chrome" ];
    environment.systemPackages = [ (pkgs.google-chrome.override { commandLineArgs = flags; }) ];
    programs.chromium = lib.mkIf (apps != {}) {
      enable = true;
      extraOpts.WebAppInstallForceList = lib.mapAttrsToList (_: app: {
        inherit (app) url;
        custom_name = app.name;
        default_launch_container = "window";
        create_desktop_shortcut = false;
      }) apps;
    };
  })
  {
    home-manager.users = lib.optionalAttrs config.settings.users.admin.homeManager.enable {
      ${config.settings.users.admin.username} = { config, lib, ... }: let
        applications = "${config.xdg.dataHome}/applications";
        command = "${launchers}/bin/chrome-launchers ${declaration} ${lib.escapeShellArg applications}";
      in {
        # Also runs with an empty declaration when Chrome/apps are disabled.
        home.activation.chromeLaunchers = lib.hm.dag.entryAfter [ "writeBoundary" ] ''
          run ${command}
        '';
        systemd.user.services.chrome-launchers = lib.mkIf (apps != {}) {
          Unit = {
            Description = "Resolve Chrome PWA taskbar launchers";
            PartOf = [ "graphical-session.target" ];
            StartLimitIntervalSec = 0;
          };
          Service = { Type = "oneshot"; ExecStart = command; };
          Install.WantedBy = [ "graphical-session.target" ];
        };
        systemd.user.paths.chrome-launchers = lib.mkIf (apps != {}) {
          Unit = {
            Description = "Watch Chrome's installed PWA launchers";
            PartOf = [ "graphical-session.target" ];
          };
          Path.PathChanged = applications;
          Install.WantedBy = [ "graphical-session.target" ];
        };
      };
    };
  }
]
