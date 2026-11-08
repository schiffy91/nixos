{ config, lib, pkgs, ... }:
let apps = config.settings.apps;
in lib.mkIf (apps.enable && apps.gaming.enable && apps.chiaki.enable) {
  environment.systemPackages = [ pkgs.chiaki-ng ];
  settings.disk.immutability.persist.paths = [ "${config.settings.users.admin.homeDirectory}/.config/Chiaki" ];
}
