{ config, inputs, lib, pkgs, pkgs-unstable, ... }:
let
  system = pkgs.stdenv.hostPlatform.system;
  user = config.settings.users.admin.username;
  btrc = inputs.btrc.packages.${system};
  vscode = pkgs-unstable.vscode-with-extensions.override {
    vscode = pkgs-unstable.vscode;
    vscodeExtensions = [ btrc.btrc-vscode ];
  };
in lib.mkIf (config.settings.apps.enable && config.settings.apps.dev.enable && config.settings.apps.vscode.enable) {
  users.users.${user}.packages = [
    vscode
    btrc.btrc-lsp
  ];
}
