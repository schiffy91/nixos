{ config, lib, pkgs, ... }:
let enabled = config.settings.apps.enable && config.settings.apps.utils.enable && config.settings.apps.desktop.enable;
in lib.mkMerge [ {
  settings.apps = lib.mkIf config.settings.apps.desktop.enable {
    chrome.enable = lib.mkDefault true;
    cider.enable = lib.mkDefault true;
    mullvad.enable = lib.mkDefault true;
  };
} (lib.mkIf enabled {
    environment.systemPackages = with pkgs; [
      (mpv.override { youtubeSupport = false; })
      solaar
      discord
    ];
    programs.openvpn3.enable = true;
    hardware.logitech.wireless.enable = true;
    services.resolved.enable = true;
}) ]
