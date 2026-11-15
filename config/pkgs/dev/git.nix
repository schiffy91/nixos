{ config, lib, pkgs, ... }: lib.mkIf (config.settings.apps.dev.git.enable) {
  programs.git.enable = true;
  environment.systemPackages = [ pkgs.gh ];  # GitHub's own tool: CI logs and artifacts, pull requests
  # Its sign-in: the account and host are here, the token is in the desktop's wallet where there is one.
  settings.disk.immutability.persist.paths = [ "${config.settings.users.admin.homeDirectory}/.config/gh" ];
}
