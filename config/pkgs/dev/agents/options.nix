{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  options = with lib.types; {
    settings.apps.agents.enable = mkSetting bool true;
    settings.apps.agents.skills = mkSetting (listOf str) (
      [ "engineering-decomposition" "taste" "humanizer" "project-structure-taste" ]
      ++ lib.optional pkgs.stdenv.hostPlatform.isLinux "nixos-init"
    );
    settings.apps.agents.rules = mkSetting bool false;
  };
}
