{ settings, inputs, pkgs, lib, config, ... }:
let
  dev = settings.apps.enable && settings.apps.dev.enable;
  shell = import ./shell.nix { inherit pkgs inputs; };
in {
  imports = [ ./agents ./lmstudio/home.nix ];
  programs.git = lib.mkIf (dev && settings.apps.git.enable) {
    enable = true;
    package = null;
    includes = map (path: { inherit path; }) settings.apps.git.includes;
    settings = {
      user = {
        name = settings.users.admin.publicName;
        email = settings.users.admin.publicEmail;
      };
      url."git@github.com:".insteadOf = "https://github.com/";
      gpg.format = "openpgp";
    };
  };
  programs.bash = lib.mkIf (dev && settings.apps.bash.enable) {
    enable = true;
    shellAliases = shell.aliases // settings.apps.shell.aliases;
    initExtra = shell.init;
  };
  programs.zsh = lib.mkIf (dev && settings.apps.zsh.enable) {
    enable = true;
    dotDir = config.home.homeDirectory; # pins ~/.zshrc against a future default move
    shellAliases = shell.aliases // settings.apps.shell.aliases;
    initContent = shell.init;
  };
}
