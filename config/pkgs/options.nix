{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  imports = [ ./dev/options.nix ./gaming/options.nix ./utils/options.nix ./de/options.nix ./hierarchy.nix ];
  options = with lib.types; {
    settings.apps.packages = mkSetting (listOf package) [];
    settings.apps.enable = mkSetting bool true;
  };
}
