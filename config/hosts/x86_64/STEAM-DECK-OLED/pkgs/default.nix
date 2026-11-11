{ config, pkgs, host, ... }:
let
  user = config.settings.users.admin.username;
  emulationRoot = "/run/media/${user}/${host.sdCard.label}/Emulation";
in {
  settings.apps = {
    gaming.packages = with pkgs; [ protonup-qt ];
    gaming.moonlight.enable = true;
    gaming.chiaki.enable = true;
    gaming.deckpad = { enable = true; role = "controller"; };  # this Deck's controls as a Steam Controller for FRACTAL-NORTH
    utils.desktop.enable = true;
    utils.nixosctl.caffeine.enable = false;
    gaming.steam.protonCustom.enable = true;
    gaming.sunshine = {
      capture = "auto";  # KWin virtual display in Plasma; KMS in Gaming Mode
    };
    gaming.battlenet.enable = true;
    gaming.rocksmith.enable = false;
    gaming.assettocorsa.enable = false;
    gaming.semu = {
      target = "steam-deck";
      root = emulationRoot;
    };
  };
}
