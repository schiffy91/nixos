{ pkgs, config, lib, inputs, buildBtrcProgram, ... }:
let
  trayDir = "${inputs.btrc}/src/stdlib/tray";
  nixosTrayBin = buildBtrcProgram {
    name = "nixos-tray";
    entry = "btrc/desktop/nixos-tray.btrc";
    nativeBuildInputs = [ pkgs.pkg-config pkgs.makeWrapper ];
    buildInputs = [ pkgs.dbus ];
    extraCFlags = [ "-I${trayDir}" ];
    extraCInputs = [ "${trayDir}/btrc_tray_linux.c" ];
    extraLibs = [ "$(pkg-config --cflags --libs dbus-1)" "-lm" "-lpthread" ];
    extraInstall = ''
      wrapProgram $out/bin/nixos-tray \
        --prefix PATH : ${lib.makeBinPath (with pkgs; [
          kdePackages.konsole
          xdg-utils
        ])}
    '';
  };
in
lib.mkIf (config.settings.apps.enable
          && config.settings.apps.nixosctl.enable
          && config.settings.desktop.enable) {
  users.users.${config.settings.users.admin.username}.packages = [ nixosTrayBin ];
  systemd.user.services.nixos-tray = {
    description = "NixOS management system tray";
    wantedBy = [ "graphical-session.target" ];
    partOf = [ "graphical-session.target" ];
    after = [ "graphical-session.target" ];
    unitConfig.ConditionUser = config.settings.users.admin.username;
    serviceConfig = {
      ExecStart = "${nixosTrayBin}/bin/nixos-tray";
      Restart = "on-failure";
      RestartSec = 3;
    };
  };
}
