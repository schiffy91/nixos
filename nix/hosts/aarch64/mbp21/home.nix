{ settings, ... }:
{
  imports = [ ../../../home ];
  home = {
    username = settings.users.admin.username;
    homeDirectory = settings.users.admin.homeDirectory;
    stateVersion = "24.11";
  };
}
