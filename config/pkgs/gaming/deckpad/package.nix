{ pkgs, inputs }:
let
  build = import ../../../lib/btrc.nix { inherit pkgs inputs; };
  src = pkgs.lib.fileset.toSource {
    root = ./.;
    fileset = pkgs.lib.fileset.fileFilter (file: file.hasExt "btrc" || file.hasExt "h" || file.name == "btrc.toml" || file.name == "btrc.lock") ./.;
  };
  # Library.Bluetooth and Library.Tray bind libdbus-1 for their Linux providers.
  dbus = {
    buildInputs = [ pkgs.dbus ];
    env.NIX_CFLAGS_COMPILE = "-I${pkgs.lib.getDev pkgs.dbus}/include/dbus-1.0 -I${pkgs.lib.getLib pkgs.dbus}/lib/dbus-1.0/include";
    linkFlags = [ "-ldbus-1" ];
  };
  # What the Deck's screen says before it goes dark: the shortcut's logo over
  # one line, as grey levels (one byte a pixel) for deckpad-screen.
  notice = { width = 1100; height = 300; };
  notices = pkgs.runCommand "deckpad-notices" { nativeBuildInputs = [ pkgs.imagemagick ]; } ''
    mkdir -p $out
    write() {
      magick -size ${toString notice.width}x${toString notice.height} xc:black \
        \( ${../artwork/deckpad/_logo.png} -resize x119 \) -gravity North -geometry +0+30 -composite \
        -font ${pkgs.montserrat}/share/fonts/otf/Montserrat-SemiBold.otf -pointsize 34 -kerning 1 -fill '#a8a8a8' \
        -gravity South -annotate +0+40 "$2" -colorspace Gray -depth 8 "gray:$out/$1.grey"
    }
    write held 'Hold  •••  for two seconds to stop'
    write waiting 'Waiting for a PC  ·  touch the screen to stop'
  '';
in {
  # The service and its host-side helper. Beyond libc it links libsodium (the
  # network link's keys and encryption) and libdbus (the Bluetooth link).
  deckpad = build {
    name = "deckpad";
    inherit src;
    programs.deckpad = "Main.btrc";
    buildInputs = dbus.buildInputs ++ [ pkgs.libsodium ];
    inherit (dbus) env;
    linkFlags = dbus.linkFlags ++ [ "-lsodium" ];
  };
  # The Deck's screen while it is a controller: one plain X11 window. It
  # stops the service with whatever systemctl the session has, else this one.
  screen = build {
    name = "deckpad-screen";
    inherit src;
    programs.deckpad-screen = "Screen.btrc";
    buildInputs = [ pkgs.libx11 pkgs.xorgproto ];
    linkFlags = [ "-lX11" ];
    wrapperArgs = [ "--add-flags" "${notices} ${toString notice.width} ${toString notice.height}" "--suffix" "PATH" ":" "${pkgs.systemd}/bin" ];
  };
  tray = build {
    name = "deckpad-tray";
    inherit src;
    programs.deckpad-tray = "Tray.btrc";
    inherit (dbus) buildInputs env linkFlags;
  };
}
