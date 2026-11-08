{ config, lib, pkgs, ... }:
let
  chrome = config.settings.apps.chrome;
  flags = chrome.flags ++ lib.optional (chrome.renderNode != null) "--render-node-override=${chrome.renderNode}";
in lib.mkIf (config.settings.apps.enable && config.settings.apps.utils.enable && chrome.enable) {
  settings.disk.immutability.persist.paths = [ "${config.settings.users.admin.homeDirectory}/.config/google-chrome" ];
  environment.systemPackages = [ (pkgs.google-chrome.override { commandLineArgs = flags; }) ];
}
