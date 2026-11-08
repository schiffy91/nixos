{ config, lib, pkgs, ... }:
let apps = config.settings.apps;
in lib.mkIf (apps.gaming.moonlight.enable) {
  environment.systemPackages = [ pkgs.moonlight-qt ];
  settings.disk.immutability.persist.paths = [ "${config.settings.users.admin.homeDirectory}/.config/Moonlight Game Streaming Project" ];
}
