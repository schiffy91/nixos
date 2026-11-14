{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  imports = [ ./drive/options.nix ./nixosctl/options.nix ./gpu-tuning/options.nix ./cpu-tuning/options.nix ];
  options = with lib.types; {
    settings.apps.utils.desktop.enable = mkSetting bool false;
    settings.apps.utils.chrome.enable = mkSetting bool config.settings.apps.utils.desktop.enable;
    settings.apps.utils.chrome.flags = mkSetting (listOf str) [
      "--ozone-platform=x11"
      "--use-angle=vulkan"
      "--enable-features=VaapiVideoDecoder,VaapiVideoEncoder"
      "--ignore-gpu-blocklist"
      "--enable-gpu-rasterization"
    ];
    settings.apps.utils.chrome.renderNode = mkSetting (nullOr str) null;
    settings.apps.utils.chrome.profile = lib.mkOption {
      type = strMatching "[^/\n]+";
      default = "Default";
      description = "Chrome profile directory used for web-app taskbar launchers (for example, Profile 2).";
    };
    settings.apps.utils.chrome.webApps = lib.mkOption {
      default = {};
      description = "Web apps installed by Chrome policy in every profile. Names must be unique so native launchers can be resolved.";
      type = attrsOf (submodule ({ name, ... }: {
        options = {
          enable = lib.mkOption {
            type = bool;
            default = config.settings.apps.utils.chrome.enable;
            description = "Install this PWA and provide its taskbar launcher.";
          };
          name = lib.mkOption { type = str; default = name; description = "App name enforced by Chrome policy."; };
          url = lib.mkOption { type = strMatching "https?://[^[:space:]]+"; description = "Web page from which Chrome installs the PWA."; };
          launcher = lib.mkOption {
            type = str;
            readOnly = true;
            default = "applications:chrome-app-${name}.desktop";
            description = "Stable taskbar reference to Chrome's installed app.";
          };
        };
      }));
    };
    settings.apps.utils.cider.enable = mkSetting bool config.settings.apps.utils.desktop.enable;
    settings.apps.utils.mullvad.enable = mkSetting bool config.settings.apps.utils.desktop.enable;
    settings.apps.utils.enable = mkSetting bool config.settings.apps.enable;
    settings.apps.utils.packages = mkSetting (listOf package) [];
    settings.apps.utils.onePassword.enable = mkSetting bool config.settings.apps.utils.enable;
  };
}
