{ config, inputs, lib, pkgs, pkgs-unstable, ... }:
let
  system = pkgs.stdenv.hostPlatform.system;
  user = config.settings.users.admin.username;
  btrc = inputs.btrc.packages.${system};
  inherit (import ../utils/nixosctl/ui/tray/lib.nix { inherit pkgs inputs; }) btrc-lsp;
in lib.mkIf (config.settings.apps.enable && config.settings.apps.dev.enable && config.settings.apps.vscode.enable) {
  settings.disk.immutability.persist.paths = [
    "${config.settings.users.admin.homeDirectory}/.config/Code"
    "${config.settings.users.admin.homeDirectory}/.vscode"
  ];
  home-manager.users.${user} = {
    home.packages = [ btrc-lsp ];
    programs.vscode = {
      enable = true;
      package = pkgs-unstable.vscode;
      mutableExtensionsDir = true;
      profiles.default.extensions = [ btrc.btrc-vscode-extension ];
    };
  };
}
