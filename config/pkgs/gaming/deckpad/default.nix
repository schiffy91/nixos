# Deck Pad is schiffy91/deckpad. This is what connects it to this configuration: the settings a host sets
# become the module's options, and what belongs to this configuration stays here: the firewall port, what an
# erased root keeps, and the Steam shortcut with its pictures.
{ config, inputs, lib, pkgs, ... }:
let
  settings = config.settings.apps.gaming.deckpad;
  artwork = import ../artwork/package.nix { inherit pkgs inputs; };
  controller = settings.enable && settings.role == "controller";
  steamHost = settings.enable && settings.role == "host" && config.settings.apps.gaming.steam.enable;
in {
  imports = [ inputs.deckpad.nixosModules.default ];
  services.deckpad = {
    inherit (settings) enable role port trustedKeys allow usb bluetooth controller;
    user = config.settings.users.admin.username;
    tray = config.settings.desktop.enable;
  };
  settings.disk.immutability.persist.paths = lib.mkIf settings.enable config.services.deckpad.statePaths;
  settings.networking.ports.tcp = lib.mkIf controller [ settings.port ];
  settings.apps.utils.nixosctl.tray.status = lib.mkIf controller { "Deck Pad".run = "deckpad status 2>&1"; };  # the Deck runs the service that answers
  # The Deck's pad becomes a pointer and keys only through Steam on the host. Steam is not kept running for
  # that: it starts in the background when the controller attaches, if it is not open already.
  services.udev.extraRules = lib.mkIf steamHost ''
    SUBSYSTEM=="usb", ENV{DEVTYPE}=="usb_device", ATTR{idVendor}=="28de", ATTR{idProduct}=="1302", TAG+="systemd", ENV{SYSTEMD_USER_WANTS}+="steam-for-controller.service"
  '';
  systemd.user.services.steam-for-controller = lib.mkIf steamHost {
    description = "Steam in the background while a Steam Controller is attached";
    unitConfig.ConditionUser = config.settings.users.admin.username;
    after = [ "graphical-session.target" ];
    serviceConfig.ExecStart = "/run/current-system/sw/bin/steam -silent";  # no window, only the tray icon; a second start hands over to the first
  };
  settings.apps.gaming.steam.shortcuts = lib.mkIf (controller && config.settings.apps.gaming.steam.enable) {
    # A stable path: Steam derives the app ID from it.
    "Deck Pad" = lib.mkDefault ({ exe = "/run/current-system/sw/bin/deckpad-toggle"; } // artwork.deckpad);
  };
}
