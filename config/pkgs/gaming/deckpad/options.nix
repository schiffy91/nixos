{ lib, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  options = with lib.types; {
    settings.apps.gaming.deckpad.enable = mkSetting bool false;
    # "controller" is the Steam Deck offering its controls; "host" is the machine that plays with them.
    settings.apps.gaming.deckpad.role = mkSetting (enum [ "controller" "host" ]) "host";
    settings.apps.gaming.deckpad.port = mkSetting port 3240;
    # Public keys of the machines on the other side, as OpenSSH lines
    # (`deckpad key` prints a machine's own; its SSH host key by default).
    # Machines listed here never need to pair by hand.
    settings.apps.gaming.deckpad.trustedKeys = mkSetting (listOf str) [];
    # Controller: the networks hosts may connect from, before any key is checked.
    settings.apps.gaming.deckpad.allow = mkSetting (listOf str) [];
    # Controller: also serve over the USB-C cable. The port must be in device
    # mode: on a Steam Deck, the BIOS option "USB Dual Role Device" set to DRD,
    # which also stops the Deck booting from a USB drive.
    settings.apps.gaming.deckpad.usb = mkSetting bool false;
    # Controller: also serve over Bluetooth. This gives the Deck's adapter the
    # controller's identity and turns off the Deck's own Bluetooth input
    # devices (BlueZ's input plugin holds the channels a controller serves).
    settings.apps.gaming.deckpad.bluetooth = mkSetting bool false;
    # Host: where the controller is.
    settings.apps.gaming.deckpad.controller = mkSetting str "STEAM-DECK-OLED.local";
  };
}
