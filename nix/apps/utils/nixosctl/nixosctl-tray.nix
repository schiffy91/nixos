{ pkgs, config, lib, inputs, ... }:
let
  btrcpy = inputs.btrc.packages.${pkgs.stdenv.hostPlatform.system}.btrcpy;
  trayDir = "${inputs.btrc}/src/stdlib/tray";
  nixosctlTrayBin = pkgs.stdenv.mkDerivation {
    name = "nixosctl-tray";
    src = ../../../..;
    nativeBuildInputs = [ btrcpy pkgs.pkg-config pkgs.makeWrapper ];
    buildInputs = [ pkgs.dbus ];
    dontConfigure = true;
    buildPhase = ''
      btrcpy --strict-imports btrc/nixosctl/tray.btrc -o nixosctl-tray.c
      $CC -std=c11 -O2 -I${trayDir} nixosctl-tray.c ${trayDir}/btrc_tray_linux.c \
        $(pkg-config --cflags --libs dbus-1) -lm -lpthread -o nixosctl-tray
    '';
    installPhase = ''
      mkdir -p $out/bin
      cp nixosctl-tray $out/bin/nixosctl-tray
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
