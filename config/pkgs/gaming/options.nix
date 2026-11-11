{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  imports = [ ./assettocorsa/options.nix ./deckpad/options.nix ./steam/options.nix ./sunshine/options.nix ./semu/options.nix ./rocksmith/options.nix ./battlenet/options.nix ./wow/options.nix ];
  options = with lib.types; {
    settings.apps.gaming.enable = mkSetting bool config.settings.apps.enable;
    settings.apps.gaming.packages = mkSetting (listOf package) [];
    settings.apps.gaming.moonlight.enable = mkSetting bool false;
    settings.apps.gaming.chiaki.enable = mkSetting bool false;
    settings.apps.gaming.cpuAffinity = mkSetting (nullOr (strMatching "[0-9]+(-[0-9]+)?(,[0-9]+(-[0-9]+)?)*")) null; # Linux CPU list; null leaves scheduling unrestricted.
    settings.apps.gaming.btrsmith.enable = mkSetting bool false;
    settings.apps.gaming.simracing.enable = mkSetting bool false;
  };
}
