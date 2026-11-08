{ config, lib, pkgs, ... }:
let apps = config.settings.apps;
in lib.mkIf (apps.gaming.chiaki.enable) {
  environment.systemPackages = [ pkgs.chiaki-ng ];
  settings.disk.immutability.persist.paths = [ "${config.settings.users.admin.homeDirectory}/.config/Chiaki" ];
}
