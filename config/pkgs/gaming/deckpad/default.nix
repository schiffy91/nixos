{ config, lib, pkgs, inputs, ... }:
let
  settings = config.settings.apps.gaming.deckpad;
  user = config.settings.users.admin.username;
  programs = import ./package.nix { inherit pkgs inputs; };
  artwork = import ../artwork/package.nix { inherit pkgs inputs; };
  deckpad = "${programs.deckpad}/bin/deckpad";
  trust = lib.concatMapStringsSep " " (line: "--trust ${lib.escapeShellArg line}") settings.trustedKeys;
  # The service writes its control token once it answers requests.
  awaitService = "${pkgs.coreutils}/bin/timeout 10 ${pkgs.bash}/bin/sh -c 'until [ -e /run/deckpad/token ]; do ${pkgs.coreutils}/bin/sleep 0.1; done'";
  toggle = pkgs.writeShellScriptBin "deckpad-toggle" ''
    if ${pkgs.systemd}/bin/systemctl is-active --quiet deckpad.service; then
      exec ${pkgs.systemd}/bin/systemctl stop deckpad.service
    fi
    ${pkgs.systemd}/bin/systemctl start deckpad.service || exit 1
    # A Deck that trusts no host yet lets the first one pair, so Game Mode
    # needs only this one shortcut.
    ${awaitService} || exit 0
    if ${deckpad} status | ${pkgs.jq}/bin/jq -e '.trusted | length == 0' > /dev/null; then exec ${deckpad} pairing 120; fi
  '';
  # Start the service if it is not running, then let new hosts pair for two
  # minutes.
  pair = pkgs.writeShellScriptBin "deckpad-pair" ''
    ${pkgs.systemd}/bin/systemctl is-active --quiet deckpad.service || ${pkgs.systemd}/bin/systemctl start deckpad.service
    ${awaitService} || exit 1
    exec ${deckpad} pairing 120
  '';
  tray = pkgs.symlinkJoin {
    name = "deckpad-tray";
    paths = [ programs.tray ];
    nativeBuildInputs = [ pkgs.makeWrapper ];
    postBuild = ''
      wrapProgram $out/bin/deckpad-tray \
        --set DECKPAD_TRAY_ICON ${artwork.deckpad.icon} \
        --prefix PATH : ${lib.makeBinPath [ toggle pair pkgs.systemd ]}
    '';
  };
  controller = settings.enable && settings.role == "controller";
  host = settings.enable && settings.role == "host";
in {
  config = lib.mkMerge [
    (lib.mkIf settings.enable {
      environment.systemPackages = [ programs.deckpad ];
      # Trusted keys and, on the Deck, the Bluetooth host to return to.
      settings.disk.immutability.persist.paths = [ "/var/lib/deckpad" ];
    })

    # The Deck: started on demand, it serves the built-in controls until a stop
    # signal or the two-second ... hold. A host attaching takes the controls.
    (lib.mkIf controller {
      environment.systemPackages = [ toggle pair ];
      settings.networking.ports.tcp = [ settings.port ];
      boot.kernelModules = lib.mkIf settings.usb [ "libcomposite" "usb_f_fs" ];
      systemd.services.deckpad = {
        description = "Steam Deck controls as a Steam Controller";
        after = [ "jupiter-controller-update.service" ] ++ lib.optional settings.bluetooth "bluetooth.service";
        wants = lib.optional settings.bluetooth "bluetooth.service";
        serviceConfig = {
          ExecStart = lib.concatStringsSep " " ([
            "${deckpad} serve --port ${toString settings.port}"
            (lib.concatMapStringsSep " " (entry: "--allow ${lib.escapeShellArg entry}") settings.allow)
            trust
          ] ++ lib.optional settings.usb "--usb" ++ lib.optional settings.bluetooth "--bluetooth hci0");
          # Whatever happened to the service, the kernel gets the controls and
          # the USB port back.
          ExecStopPost = "${deckpad} release";
          # The control token is for the people who use this machine.
          RuntimeDirectory = "deckpad";
          RuntimeDirectoryMode = "0750";
          StateDirectory = "deckpad";
          StateDirectoryMode = "0700";
          Group = config.users.users.${user}.group;
          Restart = "on-failure";
          RestartSec = 2;
        };
      };
      # Jovian's firmware check for the Deck's controller runs at boot and on
      # every switch, and talks to the pad this service takes from the kernel.
      # A check in progress holds the service back (`after` above), and while
      # the service runs the check is skipped until the next boot or switch.
      systemd.services.jupiter-controller-update = lib.mkIf (config.jovian.devices.steamdeck.autoUpdate or false) {
        serviceConfig.ExecCondition = "${pkgs.runtimeShell} -c '! ${pkgs.systemd}/bin/systemctl is-active --quiet deckpad.service'";
      };
      # As a Bluetooth controller the Deck must look like one, and BlueZ's own
      # HID host must not hold the channels a controller serves.
      hardware.bluetooth = lib.mkIf settings.bluetooth {
        enable = true;
        disabledPlugins = [ "input" ];
        settings.General = { Class = "0x002508"; DeviceID = "usb:28DE:1303:0100"; };
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
        # A stable path: Steam derives the app ID from it.
        "Deck as Controller" = lib.mkDefault ({ exe = "/run/current-system/sw/bin/deckpad-toggle"; } // artwork.deckpad);
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

    # The PC: whenever the Deck offers its controller over the network, hand
    # it to vhci_hcd, which presents it to Steam as a wired Steam Controller.
    # Over the USB-C cable or Bluetooth the PC needs nothing.
    (lib.mkIf host {
      boot.kernelModules = [ "vhci-hcd" ];
      systemd.services.deckpad-attach = {
        description = "Attach the Steam Deck's controller";
        wantedBy = [ "multi-user.target" ];
        after = [ "network-online.target" ];
        wants = [ "network-online.target" ];
        serviceConfig = {
          # Keys named in the configuration are trusted before the first attach.
          ExecStartPre = map (line: "${deckpad} trust-local ${lib.escapeShellArg line}") settings.trustedKeys;
          ExecStart = "${deckpad} attach ${lib.escapeShellArg settings.controller} --port ${toString settings.port} --watch";
          ExecStopPost = "${deckpad} detach";
          StateDirectory = "deckpad";
          StateDirectoryMode = "0700";
          Restart = "always";
          RestartSec = 5;
        };
      };
    })
  ];
}
