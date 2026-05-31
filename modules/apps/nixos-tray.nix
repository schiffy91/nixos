{ pkgs, config, lib, inputs, btrcStdlib, ... }:
# Native NixOS management system tray. Compiles the checked-in transpiled C
# (generated/nixos-tray.c, from bin/nixos-tray.btrc) together with the btrc
# stdlib's Linux StatusNotifierItem shim, and runs it as a graphical-session
# user service. Replaces the former PyQt6 tray daemon in helper.nix.
#
# Regenerate generated/nixos-tray.c with (inside the dev shell):
#   btrcpy --no-stdlib bin/nixos-tray.btrc -o build/nixos-tray.c \
#     && cp build/nixos-tray.c generated/nixos-tray.c
let
  # The stdlib comes from the precompiled archive (btrcStdlib module arg); only
  # the platform tray shim (non-vendored C) still comes from the btrc source tree.
  trayDir = "${inputs.btrc}/src/stdlib/tray";
  nixosTrayBin = pkgs.stdenv.mkDerivation {
    name = "nixos-tray";
    dontUnpack = true;
    nativeBuildInputs = [ pkgs.pkg-config pkgs.makeWrapper ];
    buildInputs = [ pkgs.dbus ];
    buildPhase = ''
      $CC -std=c11 -O2 -I${btrcStdlib.incDir} -I${trayDir} ${btrcStdlib.gcFlags} \
        ${../../generated/nixos-tray.c} ${trayDir}/btrc_tray_linux.c ${btrcStdlib.stdlibInput} \
        $(pkg-config --cflags --libs dbus-1) -lm -lpthread -o nixos-tray
    '';
    installPhase = ''
      mkdir -p $out/bin
      cp nixos-tray $out/bin/nixos-tray
      wrapProgram $out/bin/nixos-tray \
        --prefix PATH : ${lib.makeBinPath (with pkgs; [
          kdePackages.konsole
          xdg-utils
        ])}
    '';
  };
in
lib.mkIf (config.settings.apps.enable
          && config.settings.apps.nixosHelper.enable
          && config.settings.desktop.enable) {
  environment.systemPackages = [ nixosTrayBin ];
  systemd.user.services.nixos-tray = {
    description = "NixOS management system tray";
    wantedBy = [ "graphical-session.target" ];
    partOf = [ "graphical-session.target" ];
    after = [ "graphical-session.target" ];
    serviceConfig = {
      ExecStart = "${nixosTrayBin}/bin/nixos-tray";
      Restart = "on-failure";
      RestartSec = 3;
    };
  };
}
