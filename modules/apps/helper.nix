{ pkgs, ... }:
let
  src = ../../scripts;  # ship the whole tree so cli/daemon can `from lib import …`
  cliPython = pkgs.python3;
  daemonPython = pkgs.python3.withPackages (ps: with ps; [ pyqt6 ]);
  cli = pkgs.writeShellApplication {
    name = "nixos-helper-cli";
    runtimeInputs = with pkgs; [ kdePackages.libkscreen systemd kdePackages.konsole pulseaudio ];
    text = ''exec ${cliPython}/bin/python3 ${src}/bin/nixos-helper/cli.py "$@"'';
  };
  daemon = pkgs.writeShellApplication {
    name = "nixos-helper";
    runtimeInputs = [ cli ];
    text = ''exec ${daemonPython}/bin/python3 ${src}/bin/nixos-helper/daemon.py "$@"'';
  };
  desktopItem = pkgs.makeDesktopItem {
    name = "nixos-helper";
    desktopName = "NixOS Helper";
    exec = "${daemon}/bin/nixos-helper";
    icon = "nix-snowflake";
    categories = [ "Utility" "System" ];
    comment = "Tray utilities for NixOS";
    terminal = false;
    startupWMClass = "nixos-helper";
  };
in {
  environment.systemPackages = [ cli daemon desktopItem ];
  environment.etc."xdg/autostart/nixos-helper.desktop".text = ''
    [Desktop Entry]
    Type=Application
    Name=NixOS Helper
    Exec=${daemon}/bin/nixos-helper
    Icon=nix-snowflake
    X-KDE-autostart-after=panel
    NoDisplay=true
  '';
}
