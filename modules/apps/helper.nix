{ config, pkgs, lib, ... }:
let
  pythonEnv = pkgs.python3.withPackages (ps: with ps; [ pyqt6 ]);
  src = ../../scripts/bin/nixos_helper.py;
  helper = pkgs.writeShellApplication {
    name = "nixos-helper";
    runtimeInputs = with pkgs; [ pythonEnv kdePackages.libkscreen systemd kdePackages.konsole ];
    text = ''exec ${pythonEnv}/bin/python3 ${src} "$@"'';
  };
  desktopItem = pkgs.makeDesktopItem {
    name = "nixos-helper";
    desktopName = "NixOS Helper";
    exec = "${helper}/bin/nixos-helper";
    icon = "nix-snowflake";
    categories = [ "Utility" "System" ];
    comment = "Tray utilities for NixOS";
    terminal = false;
    startupWMClass = "nixos-helper";
  };
in {
  environment.systemPackages = [ helper desktopItem ];
  environment.etc."xdg/autostart/nixos-helper.desktop".text = ''
    [Desktop Entry]
    Type=Application
    Name=NixOS Helper
    Exec=${helper}/bin/nixos-helper
    Icon=nix-snowflake
    X-KDE-autostart-after=panel
    NoDisplay=true
  '';
}
