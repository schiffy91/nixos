{ settings, lib, config, ... }:
let dev = settings.apps.enable && settings.apps.dev.enable;
in {
  imports = [ ./agents ];
  programs.home-manager.enable = true;
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
    shellAliases = settings.apps.shell.aliases;
    initExtra = builtins.readFile ./shell.sh;
  };
  programs.zsh = lib.mkIf (dev && settings.apps.zsh.enable) {
    enable = true;
    dotDir = config.home.homeDirectory; # pins ~/.zshrc against a future default move
    shellAliases = settings.apps.shell.aliases;
    initContent = builtins.readFile ./shell.sh;
  };
}
