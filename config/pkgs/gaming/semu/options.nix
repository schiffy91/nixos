{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  options = with lib.types; {
    settings.apps.gaming.semu.enable = mkSetting bool (config.settings.apps.gaming.enable && pkgs.stdenv.hostPlatform.isx86_64);
    settings.apps.gaming.semu.target = mkSetting str "linux-desktop";
    settings.apps.gaming.semu.root = mkSetting (nullOr str) null; # the library root; null keeps the target's own
    settings.apps.gaming.semu.drive = mkSetting (nullOr str) "media/Games/Emulation"; # Drive folder the library syncs with; null keeps it local
  };
}
