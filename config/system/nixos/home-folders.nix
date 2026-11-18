{ config, lib, pkgs, ... }:
let
  user = config.settings.users.admin.username;
  group = config.users.users.${user}.group;
  home = config.settings.users.admin.homeDirectory;
in {
  system.activationScripts.userFolder = lib.stringAfter [ "users" ] ''
    user_folder() {  # as root: `install -d` alone leaves the folders above to root
      case "$1" in ${home}/*/*) user_folder "''${1%/*}";; esac
      [ -d "$1" ] || ${pkgs.coreutils}/bin/install -d -o ${user} -g ${group} "$1"
      ${pkgs.coreutils}/bin/chown ${user}:${group} "$1"
    }
  '';
}
