{ config, inputs, lib, pkgs, pkgs-unstable, ... }:
let
  system = pkgs.stdenv.hostPlatform.system;
  user = config.settings.users.admin.username;
in lib.mkIf (config.settings.apps.enable && config.settings.apps.vscode.enable) {
  users.users.${user}.packages = [
    pkgs-unstable.vscode
    inputs.btrc.packages.${system}.btrc-lsp
  ];
}
