{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
  adminHome = "/home/${config.settings.users.admin.username}";
  rocksmith = config.settings.apps.gaming.rocksmith;
in {
  options = with lib.types; {
    settings.apps.gaming.rocksmith.enable = mkSetting bool config.settings.apps.gaming.steam.protonSge.enable;
    settings.apps.gaming.rocksmith.sampleSize = mkSetting int 64;
    settings.apps.gaming.rocksmith.sampleRate = mkSetting int 48000;
    settings.apps.gaming.rocksmith.drive = mkSetting (nullOr str) (if config.settings.apps.utils.drive.enable then "media/Games/Rocksmith" else null);  # Drive folder that holds the songs; null keeps them on this machine
    settings.apps.gaming.rocksmith.songsPath = mkSetting str (if rocksmith.drive != null then "${config.settings.apps.utils.drive.mountPoint}/${rocksmith.drive}" else "${adminHome}/Games/Rocksmith/cdlc");  # every song below it is linked into the game
    settings.apps.gaming.rocksmith.slopsmith.configPath = mkSetting str "${adminHome}/Games/Rocksmith/slopsmith/config";
    settings.apps.gaming.rocksmith.slopsmith.port = mkSetting port 8000;
  };
}
