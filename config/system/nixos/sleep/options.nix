{ lib, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  options = with lib.types; {
    settings.sleep.lockGamingSession = mkSetting bool false;
    settings.sleep.suspendMode = mkSetting (enum [ "s2idle" "deep" ]) "s2idle";
    settings.sleep.allowHibernation = mkSetting bool false;
    settings.sleep.hibernateOnSuspend = mkSetting bool false; # implies allowHibernation; use hibernation for suspend requests
  };
}
