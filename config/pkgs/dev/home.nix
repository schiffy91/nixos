{ settings, inputs, pkgs, lib, config, ... }:
let
  shell = import ./shell.nix { inherit pkgs inputs; };
in {
  imports = [ ./agents ./lmstudio/home.nix ];
  programs.git = lib.mkIf (settings.apps.dev.git.enable) {
    enable = true;
    package = null;
    includes = map (path: { inherit path; }) settings.apps.dev.git.includes;
    settings = {
      user = {
        name = settings.users.admin.publicName;
        email = settings.users.admin.publicEmail;
      };
      url."git@github.com:".insteadOf = "https://github.com/";
      gpg.format = "openpgp";
    };
  };
  programs.bash = lib.mkIf (settings.apps.dev.bash.enable) {
    enable = true;
    shellAliases = shell.aliases // settings.apps.dev.shell.aliases;
    initExtra = shell.init;
  };
  programs.zsh = lib.mkIf (settings.apps.dev.zsh.enable) {
    enable = true;
    dotDir = config.home.homeDirectory; # pins ~/.zshrc against a future default move
    shellAliases = shell.aliases // settings.apps.dev.shell.aliases;
    initContent = shell.init;
  };
}
