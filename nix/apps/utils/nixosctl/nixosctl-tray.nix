{ pkgs, config, lib, inputs, buildBtrcProgram, ... }:
let
  trayDir = "${inputs.btrc}/src/stdlib/tray";
  nixosctlTrayBin = buildBtrcProgram {
    name = "nixosctl-tray";
    entry = "btrc/desktop/nixosctl-tray.btrc";
    nativeBuildInputs = [ pkgs.pkg-config pkgs.makeWrapper ];
    buildInputs = [ pkgs.dbus ];
    extraCFlags = [ "-I${trayDir}" ];
    extraCInputs = [ "${trayDir}/btrc_tray_linux.c" ];
    extraLibs = [ "$(pkg-config --cflags --libs dbus-1)" "-lm" "-lpthread" ];
    extraInstall = ''
      install -Dm644 ${pkgs.nixos-icons}/share/icons/hicolor/scalable/apps/nix-snowflake.svg \
        $out/share/icons/hicolor/scalable/apps/nixosctl-tray.svg
      install -Dm644 ${pkgs.nixos-icons}/share/icons/hicolor/256x256/apps/nix-snowflake.png \
        $out/share/icons/hicolor/256x256/apps/nixosctl-tray.png
      wrapProgram $out/bin/nixosctl-tray \
        --set NIXOSCTL_TRAY_ICON "$out/share/icons/hicolor/256x256/apps/nixosctl-tray.png" \
        --prefix PATH : ${lib.makeBinPath (with pkgs; [
          kdePackages.konsole
          xdg-utils
        ])}
    '';
  };
in
lib.mkIf (config.settings.apps.enable
          && config.settings.apps.utils.enable
          && config.settings.apps.nixosctl.enable
          && config.settings.desktop.enable) {
  users.users.${config.settings.users.admin.username}.packages = [ nixosctlTrayBin ];
  systemd.user.services.nixosctl-tray = {
    description = "NixOS management system tray";
    wantedBy = [ "graphical-session.target" ];
    partOf = [ "graphical-session.target" ];
    after = [ "graphical-session.target" ];
    unitConfig.ConditionUser = config.settings.users.admin.username;
    serviceConfig = {
      ExecStart = "${nixosctlTrayBin}/bin/nixosctl-tray";
      Restart = "on-failure";
      RestartSec = 3;
    };
  };
}
