{ config, lib, pkgs, inputs, ... }:
let
  settings = config.settings.apps.gaming.deckpad;
  user = config.settings.users.admin.username;
  programs = import ./package.nix { inherit pkgs inputs; };
  deckpad = "${programs.deckpad}/bin/deckpad";
  toggle = pkgs.writeShellScriptBin "deckpad-toggle" ''
    if ${pkgs.systemd}/bin/systemctl is-active --quiet deckpad.service; then
      exec ${pkgs.systemd}/bin/systemctl stop deckpad.service
    fi
    exec ${pkgs.systemd}/bin/systemctl start deckpad.service
  '';
  tray = pkgs.symlinkJoin {
    name = "deckpad-tray";
    paths = [ programs.tray ];
    nativeBuildInputs = [ pkgs.makeWrapper ];
    postBuild = ''
      wrapProgram $out/bin/deckpad-tray \
        --set DECKPAD_TRAY_ICON ${pkgs.kdePackages.oxygen-icons}/share/icons/oxygen/base/64x64/devices/input-gaming.png \
        --prefix PATH : ${lib.makeBinPath [ toggle pkgs.systemd ]}
    '';
  };
  controller = settings.enable && settings.role == "controller";
  host = settings.enable && settings.role == "host";
in {
  config = lib.mkMerge [
    (lib.mkIf settings.enable { environment.systemPackages = [ programs.deckpad ]; })

    # The Deck: started on demand, it serves the built-in controls until a stop
    # signal or the two-second ... hold. A host attaching takes the controls.
    (lib.mkIf controller {
      environment.systemPackages = [ toggle ];
      settings.networking.ports.tcp = [ settings.port ];
      systemd.services.deckpad = {
        description = "Steam Deck controls as a Steam Controller";
        serviceConfig = {
          ExecStart = "${deckpad} serve --listen 0.0.0.0 --port ${toString settings.port} ${lib.concatMapStringsSep " " (entry: "--allow ${lib.escapeShellArg entry}") settings.allow}";
          # Whatever happened to the service, usbhid gets the controls back.
          ExecStopPost = "${deckpad} release";
          Restart = "on-failure";
          RestartSec = 2;
        };
      };
      # The admin switches the service without a password, in Game Mode too.
      security.polkit.extraConfig = ''
        polkit.addRule(function(action, subject) {
          if (action.id == "org.freedesktop.systemd1.manage-units" &&
              action.lookup("unit") == "deckpad.service" && subject.user == "${user}") {
            return polkit.Result.YES;
          }
        });
      '';
      settings.apps.gaming.steam.shortcuts = lib.mkIf config.settings.apps.gaming.steam.enable {
        "Deck as Controller" = lib.mkDefault "/run/current-system/sw/bin/deckpad-toggle";  # stable: Steam derives the app ID from it
      };
      systemd.user.services.deckpad-tray = lib.mkIf config.settings.desktop.enable {
        description = "Deck as Controller tray switch";
        wantedBy = [ "graphical-session.target" ];
        partOf = [ "graphical-session.target" ];
        after = [ "graphical-session.target" ];
        unitConfig.ConditionUser = user;
        serviceConfig = {
          ExecStart = "${tray}/bin/deckpad-tray";
          Restart = "on-failure";
          RestartSec = 3;
        };
      };
    })

    # The PC: whenever the Deck offers its controller, hand it to vhci_hcd,
    # which presents it to Steam as a wired USB Steam Controller.
    (lib.mkIf host {
      boot.kernelModules = [ "vhci-hcd" ];
      systemd.services.deckpad-attach = {
        description = "Attach the Steam Deck's controller";
        wantedBy = [ "multi-user.target" ];
        after = [ "network-online.target" ];
        wants = [ "network-online.target" ];
        serviceConfig = {
          ExecStart = "${deckpad} attach ${lib.escapeShellArg settings.controller} --port ${toString settings.port} --watch";
          ExecStopPost = "${deckpad} detach";
          Restart = "always";
          RestartSec = 5;
        };
      };
    })
  ];
}
