{ config, pkgs, lib, ... }: let
  user = config.settings.user.admin.username;
  src = config.settings.desktop.cursor.path;
in {
  system.activationScripts.cursorIcons = lib.stringAfter [ "users" ] ''
    ${pkgs.coreutils}/bin/install -d -o ${user} /home/${user}/.icons /home/${user}/.local/share/icons
    ${pkgs.coreutils}/bin/ln -sfn ${src} /home/${user}/.icons/default
    ${pkgs.coreutils}/bin/ln -sfn ${src} /home/${user}/.local/share/icons/default
    ${pkgs.coreutils}/bin/chown -h ${user} /home/${user}/.icons/default /home/${user}/.local/share/icons/default
  '';
}
