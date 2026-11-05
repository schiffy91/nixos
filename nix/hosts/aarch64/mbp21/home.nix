{ settings, lib, ... }:
{
  imports = [ ../../../home ];
  home = {
    username = settings.users.admin.username;
    homeDirectory = settings.users.admin.homeDirectory;
    stateVersion = "24.11";
  };
  programs.zsh = {
    initContent = lib.mkOrder 550 ''
      export PATH="$PATH:${settings.users.admin.homeDirectory}/.lmstudio/bin"
      . "$HOME/.local/bin/env"
    '';
    profileExtra = "setopt no_nomatch"; # let patterns with no matches pass through
  };
}
