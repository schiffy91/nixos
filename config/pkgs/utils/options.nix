{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  imports = [ ./nixosctl/options.nix ];
  options = with lib.types; {
    settings.apps.desktop.enable = mkSetting bool false;
    settings.apps.chrome.enable = mkSetting bool false;
    settings.apps.chrome.flags = mkSetting (listOf str) [
      "--ozone-platform=x11"
      "--use-angle=vulkan"
      "--enable-features=VaapiVideoDecoder,VaapiVideoEncoder"
      "--ignore-gpu-blocklist"
      "--enable-gpu-rasterization"
    ];
    settings.apps.chrome.renderNode = mkSetting (nullOr str) null;
    settings.apps.cider.enable = mkSetting bool false;
    settings.apps.mullvad.enable = mkSetting bool false;
    settings.apps.utils.enable = mkSetting bool true;
    settings.apps.utils.packages = mkSetting (listOf package) [];
    settings.apps.onePassword.enable = mkSetting bool true;
    settings.apps.rclone.enable = mkSetting bool true;
  };
}
