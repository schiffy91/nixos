{ config, pkgs, lib, inputs, ... }:
let
  enabled = config.settings.apps.enable
    && config.settings.apps.gaming.enable
    && config.settings.apps.semu.enable
    && pkgs.stdenv.hostPlatform.isx86_64;
  user = config.settings.users.admin.username;
  group = config.users.users.${user}.group;
  home = "/home/${user}";
  target = config.settings.semu.target;
  semu = inputs.semu.packages.${pkgs.stdenv.hostPlatform.system}.semu;
in lib.mkIf enabled {
  environment.systemPackages = [ semu ];
  environment.sessionVariables.SEMU_TARGET = target;

  system.activationScripts.semu = lib.stringAfter [ "users" ] ''
    install -d -o ${user} -g ${group} "${home}/ES-DE" "${home}/.local/share/semu" "${home}/.config/semu"
    ${pkgs.util-linux}/bin/runuser -u ${user} -- env HOME="${home}" SEMU_TARGET="${target}" \
      ${semu}/bin/semu prepare --target "${target}" || true
  '';
}
