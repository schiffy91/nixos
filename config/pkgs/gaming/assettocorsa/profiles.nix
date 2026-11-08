{ config, lib, ... }:
let
  assetto = config.settings.apps.gaming.assettocorsa;
in {
  settings.apps.gaming.assettocorsa = lib.mkMerge [
    (lib.mkIf (assetto.presetName != null && assetto.contentDirectory != null) {
      presets = lib.mkDefault [
        { name = "Video Settings/${assetto.presetName}.cmpreset"; source = "${assetto.contentDirectory}/${assetto.presetName}.cmpreset"; }
        { name = "Audio Settings/${assetto.presetName}.cmpreset"; source = "${assetto.contentDirectory}/Audio/${assetto.presetName}.cmpreset"; }
        { name = "Controls/${assetto.presetName}.ini"; source = "${assetto.contentDirectory}/Controls/${assetto.presetName}.ini"; }
      ];
    })
    (lib.mkIf (assetto.graphicsPreset == "pure-lcs-medium") {
      gameSettings = lib.mapAttrsRecursive (_: lib.mkDefault) (import ./pure-lcs-medium.nix);
    })
    (lib.mkIf (assetto.wheelProfile == "logitech-pro-playstation") {
      controlBindings = lib.mkDefault (import ./logitech-pro-playstation.nix {
        inherit lib;
        wheelMenu = assetto.wheelMenu.enable;
      });
    })
  ];
}
