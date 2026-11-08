{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  options = with lib.types; {
    settings.apps.sunshine.enable = mkSetting bool true;
    settings.apps.sunshine.sshLogin.enable = mkSetting bool false;
    settings.apps.sunshine.sshLogin.publicKeys = mkSetting (listOf str) config.settings.users.admin.authorizedKeys;
    settings.apps.sunshine.credentials.usernameReference = mkSetting str "op://Private/Sunshine ${config.networking.hostName}/username";
    settings.apps.sunshine.credentials.passwordReference = mkSetting str "op://Private/Sunshine ${config.networking.hostName}/password";
    settings.apps.sunshine.capture = mkSetting (enum [ "kms" "kwin" "auto" ]) "kms";
    settings.apps.sunshine.cuda.enable = mkSetting bool false;
    settings.apps.sunshine.virtualDisplay.enable = mkSetting bool false;
    settings.apps.sunshine.virtualDisplay.name = mkSetting str "sunshine-vmon";
    settings.apps.sunshine.virtualDisplay.disablePrimaryOnStream = mkSetting bool false;
    settings.apps.sunshine.virtualDisplay.primaryOutput = mkSetting str "";
    settings.apps.sunshine.virtualDisplay.position = mkSetting str "0,0";
    settings.apps.sunshine.virtualDisplay.scale = mkSetting str "1";
    settings.apps.sunshine.virtualDisplay.port = mkSetting port 5905;
    settings.apps.sunshine.virtualDisplay.defaultWidth = mkSetting ints.positive 1920;
    settings.apps.sunshine.virtualDisplay.defaultHeight = mkSetting ints.positive 1080;
    settings.apps.sunshine.virtualDisplay.defaultFps = mkSetting ints.positive 60;
    settings.apps.sunshine.virtualDisplay.settleSeconds = mkSetting ints.positive 3;
  };
}
