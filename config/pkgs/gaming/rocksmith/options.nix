{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
  adminHome = "/home/${config.settings.users.admin.username}";
in {
  options = with lib.types; {
    settings.apps.rocksmith.enable = mkSetting bool true;
    settings.apps.rocksmith.sampleSize = mkSetting int 64;
    settings.apps.rocksmith.sampleRate = mkSetting int 48000;
    settings.apps.rocksmith.cdlcPath = mkSetting str "${adminHome}/Games/Rocksmith/cdlc";
    settings.apps.rocksmith.slopsmith.configPath = mkSetting str "${adminHome}/Games/Rocksmith/slopsmith/config";
    settings.apps.rocksmith.slopsmith.port = mkSetting port 8000;
  };
}
