{ config, lib, ... }:
# The user-facing CLI is now the packaged `nixosctl` binary (see
# modules/apps/nixosctl.nix), which transpiles bin/nixosctl.btrc and replaces
# the former Python `nixos` CLI that lived here.
#
# TODO: the PyQt6 system tray daemon that used to be defined here (and its
# xdg/autostart entry) has been removed. A native BTRC systray (currently being
# built in the btrc stdlib) will replace it; wire it up here once available.
lib.mkIf (config.settings.apps.enable && config.settings.apps.nixosHelper.enable) {
}
