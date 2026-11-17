# Deck Pad is schiffy91/deckpad. This is what connects it to this configuration: the settings a host sets
# become the module's options, and what belongs to this configuration stays here: the firewall port, what an
# erased root keeps, and the Steam shortcut with its pictures.
{ config, inputs, lib, pkgs, ... }:
let
  settings = config.settings.apps.gaming.deckpad;
  artwork = import ../artwork/package.nix { inherit pkgs inputs; };
  controller = settings.enable && settings.role == "controller";
in {
  imports = [ inputs.deckpad.nixosModules.default ];
  services.deckpad = {
    inherit (settings) enable role port trustedKeys allow usb bluetooth controller;
    user = config.settings.users.admin.username;
    tray = config.settings.desktop.enable;
  };
  settings.disk.immutability.persist.paths = lib.mkIf settings.enable config.services.deckpad.statePaths;
  settings.networking.ports.tcp = lib.mkIf controller [ settings.port ];
  settings.apps.gaming.steam.shortcuts = lib.mkIf (controller && config.settings.apps.gaming.steam.enable) {
    # A stable path: Steam derives the app ID from it.
    "Deck Pad" = lib.mkDefault ({ exe = "/run/current-system/sw/bin/deckpad-toggle"; } // artwork.deckpad);
  };
}
