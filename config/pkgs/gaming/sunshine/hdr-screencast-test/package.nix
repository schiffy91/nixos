{ pkgs }:
pkgs.runCommandCC "hdr-screencast-probe" {
  nativeBuildInputs = [ pkgs.pkg-config pkgs.wayland-scanner ];
  buildInputs = [ pkgs.wayland pkgs.wayland-protocols pkgs.pipewire pkgs.libdrm ];
  XDG_DATA_DIRS = "${pkgs.kdePackages.plasma-wayland-protocols}/share";
} ''
  wayland-scanner client-header ${pkgs.kdePackages.plasma-wayland-protocols}/share/plasma-wayland-protocols/zkde-screencast-unstable-v1.xml zkde-screencast-unstable-v1-client-protocol.h
  wayland-scanner private-code ${pkgs.kdePackages.plasma-wayland-protocols}/share/plasma-wayland-protocols/zkde-screencast-unstable-v1.xml zkde-screencast-unstable-v1-protocol.c
  wayland-scanner client-header ${pkgs.wayland-protocols}/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell-client-protocol.h
  wayland-scanner private-code ${pkgs.wayland-protocols}/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml xdg-shell-protocol.c
  $CC -O1 -g -Wall -Wno-unused-function -I. -o probe ${./probe.c} \
    zkde-screencast-unstable-v1-protocol.c xdg-shell-protocol.c \
    $(pkg-config --cflags --libs wayland-client libpipewire-0.3 libdrm)
  install -Dm755 probe "$out/bin/hdr-screencast-probe"
''
