{ lib, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  options = with lib.types; {
    settings.apps.gaming.deckpad.enable = mkSetting bool false;
    # "controller" is the Steam Deck offering its controls; "host" is the machine that plays with them.
    settings.apps.gaming.deckpad.role = mkSetting (enum [ "controller" "host" ]) "host";
    settings.apps.gaming.deckpad.port = mkSetting port 3240;  # the USB/IP port, so stock `usbip` can attach too
    # Controller: who may attach. USB/IP has no authentication, so the firewall's
    # trusted networks decide who reaches the port and this list can narrow it.
    settings.apps.gaming.deckpad.allow = mkSetting (listOf str) [ "0.0.0.0/0" ];
    # Host: where the controller is.
    settings.apps.gaming.deckpad.controller = mkSetting str "STEAM-DECK-OLED.local";
  };
}
