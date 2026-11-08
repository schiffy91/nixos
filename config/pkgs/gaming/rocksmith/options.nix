{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
  adminHome = "/home/${config.settings.users.admin.username}";
in {
  options = with lib.types; {
    settings.apps.gaming.rocksmith.enable = mkSetting bool config.settings.apps.gaming.steam.protonCustom.enable;
    settings.apps.gaming.rocksmith.sampleSize = mkSetting int 64;
    settings.apps.gaming.rocksmith.sampleRate = mkSetting int 48000;
    settings.apps.gaming.rocksmith.cdlcPath = mkSetting str "${adminHome}/Games/Rocksmith/cdlc";
    settings.apps.gaming.rocksmith.slopsmith.configPath = mkSetting str "${adminHome}/Games/Rocksmith/slopsmith/config";
    settings.apps.gaming.rocksmith.slopsmith.port = mkSetting port 8000;
  };
}
