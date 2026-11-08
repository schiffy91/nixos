{ config, lib, pkgs, ... }:
let apps = config.settings.apps;
in lib.mkIf (apps.enable && apps.dev.enable && apps.distrobox.enable) {
  environment.systemPackages = [ pkgs.distrobox ];
  settings.disk.immutability.persist.paths = [ "${config.settings.users.admin.homeDirectory}/.local/share/containers" ];
}
