{ config, lib, pkgs-unstable, ... }:
let
  user = config.settings.users.admin.username;
in lib.mkIf (config.settings.apps.enable && config.settings.apps.vscode.enable) {
  users.users.${user}.packages = [ pkgs-unstable.vscode ];
}
