{ config, inputs, lib, pkgs, pkgs-unstable, ... }:
let
  system = pkgs.stdenv.hostPlatform.system;
  user = config.settings.users.admin.username;
  btrc = inputs.btrc.packages.${system};
in lib.mkIf (config.settings.apps.enable && config.settings.apps.dev.enable && config.settings.apps.vscode.enable) {
  home-manager.users.${user} = {
    home.packages = [ btrc.btrc-lsp ];
    programs.vscode = {
      enable = true;
      package = pkgs-unstable.vscode;
      mutableExtensionsDir = true;
      profiles.default.extensions = [ btrc.btrc-vscode ];
    };
  };
}
