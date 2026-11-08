{ config, lib, pkgs, ... }:
lib.mkIf config.settings.apps.utils.desktop.enable {
    environment.systemPackages = with pkgs; [
      (mpv.override { youtubeSupport = false; })
      solaar
      discord
    ];
    programs.openvpn3.enable = true;
    hardware.logitech.wireless.enable = true;
    services.resolved.enable = true;
}
