{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  options = with lib.types; {
    settings.apps.dev.agents.enable = mkSetting bool config.settings.apps.dev.enable;
    settings.apps.dev.agents.skills = mkSetting (listOf str) (
      [ "engineering-decomposition" "taste" "humanizer" "project-structure-taste" ]
      ++ lib.optional pkgs.stdenv.hostPlatform.isLinux "nixos-init"
    );
    settings.apps.dev.agents.rules = mkSetting bool false;
  };
}
