# VPN is schiffy91/vpn. This is what connects it to this configuration: the program on a machine that asks
# for it, what an erased root keeps, and the Steam shortcut with its icon.
{ config, inputs, lib, ... }:
let
  enable = config.settings.apps.utils.vpn.enable;
in {
  imports = [ inputs.vpn.nixosModules.default ];
  programs.vpn.enable = enable;
  settings.disk.immutability.persist.paths = lib.mkIf enable [ "${config.settings.users.admin.homeDirectory}/.config/vpn" ];  # where the chosen file is
  settings.apps.gaming.steam.shortcuts = lib.mkIf (enable && config.settings.apps.gaming.steam.enable) {
    # A stable path: Steam derives the app ID from it.
    VPN = lib.mkDefault { exe = "/run/current-system/sw/bin/vpn"; icon = "${config.programs.vpn.icon}"; };
  };
}
