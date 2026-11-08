{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  options = with lib.types; {
    settings.apps.gaming.sunshine.enable = mkSetting bool config.settings.apps.gaming.enable;
    settings.apps.gaming.sunshine.sshLogin.enable = mkSetting bool (config.settings.apps.gaming.sunshine.enable && config.settings.apps.utils.desktop.enable);
    settings.apps.gaming.sunshine.sshLogin.publicKeys = mkSetting (listOf str) config.settings.users.admin.authorizedKeys;
    settings.apps.gaming.sunshine.credentials.usernameReference = mkSetting str "op://Private/Sunshine ${config.networking.hostName}/username";
    settings.apps.gaming.sunshine.credentials.passwordReference = mkSetting str "op://Private/Sunshine ${config.networking.hostName}/password";
    settings.apps.gaming.sunshine.capture = mkSetting (enum [ "kms" "kwin" "auto" ]) "kms";
    settings.apps.gaming.sunshine.cuda.enable = mkSetting bool false;
    settings.apps.gaming.sunshine.virtualDisplay.enable = mkSetting bool (config.settings.apps.gaming.sunshine.enable && config.settings.apps.utils.desktop.enable);
    settings.apps.gaming.sunshine.virtualDisplay.name = mkSetting str "sunshine-vmon";
    settings.apps.gaming.sunshine.virtualDisplay.disablePrimaryOnStream = mkSetting bool config.settings.apps.utils.desktop.enable;
    settings.apps.gaming.sunshine.virtualDisplay.primaryOutput = mkSetting str "";
    settings.apps.gaming.sunshine.virtualDisplay.position = mkSetting str "0,0";
    settings.apps.gaming.sunshine.virtualDisplay.scale = mkSetting str "1";
    settings.apps.gaming.sunshine.virtualDisplay.port = mkSetting port 5905;
    settings.apps.gaming.sunshine.virtualDisplay.defaultWidth = mkSetting ints.positive 1920;
    settings.apps.gaming.sunshine.virtualDisplay.defaultHeight = mkSetting ints.positive 1080;
    settings.apps.gaming.sunshine.virtualDisplay.defaultFps = mkSetting ints.positive 60;
    settings.apps.gaming.sunshine.virtualDisplay.settleSeconds = mkSetting ints.positive 3;
  };
}
