{ pkgs, inputs }:
let
  build = import ../../../lib/btrc.nix { inherit pkgs inputs; };
  src = pkgs.lib.fileset.toSource {
    root = ./.;
    fileset = pkgs.lib.fileset.fileFilter (file: file.hasExt "btrc" || file.name == "btrc.toml" || file.name == "btrc.lock") ./.;
  };
  # Library.Bluetooth and Library.Tray bind libdbus-1 for their Linux providers.
  dbus = {
    buildInputs = [ pkgs.dbus ];
    env.NIX_CFLAGS_COMPILE = "-I${pkgs.lib.getDev pkgs.dbus}/include/dbus-1.0 -I${pkgs.lib.getLib pkgs.dbus}/lib/dbus-1.0/include";
    linkFlags = [ "-ldbus-1" ];
  };
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
  tray = build {
    name = "deckpad-tray";
    inherit src;
    programs.deckpad-tray = "Tray.btrc";
    inherit (dbus) buildInputs env linkFlags;
  };
}
