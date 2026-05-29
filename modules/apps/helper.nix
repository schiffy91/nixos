{ config, lib, ... }:
# The user-facing CLI is now the packaged `nixosctl` binary (see
# modules/apps/nixosctl.nix), which transpiles bin/nixosctl.btrc and replaces
# the former Python `nixos` CLI that lived here.
#
# The PyQt6 system tray daemon that used to be defined here (and its
# xdg/autostart entry) has been replaced by the native BTRC systray, packaged
# and run as a graphical-session user service in modules/apps/nixos-tray.nix.
lib.mkIf (config.settings.apps.enable && config.settings.apps.nixosHelper.enable) {
}
