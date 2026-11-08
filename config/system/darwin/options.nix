{ lib, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  options = with lib.types; {
    settings.macos.nix.enable = mkSetting bool false; # Determinate owns the daemon.
    settings.macos.firewall.enable = mkSetting bool true;
    settings.macos.firewall.blockAllIncoming = mkSetting bool false;
    settings.macos.firewall.allowSigned = mkSetting bool true;
    settings.macos.firewall.allowSignedApp = mkSetting bool true;
    settings.macos.firewall.enableStealthMode = mkSetting bool false;
    settings.macos.guest.enable = mkSetting bool false;
    settings.macos.screenLock.requirePassword = mkSetting bool true;
    settings.macos.screenLock.delay = mkSetting int 0;
    settings.macos.updates.automaticallyInstall = mkSetting bool true;
    settings.macos.permissions = mkSetting attrs {};
  };
}
