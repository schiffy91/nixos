{ pkgs, inputs }:
let
  build = import ../../../lib/btrc.nix { inherit pkgs inputs; };
  src = pkgs.lib.fileset.toSource {
    root = ./.;
    fileset = pkgs.lib.fileset.fileFilter (file: file.hasExt "btrc" || file.hasExt "h" || file.name == "btrc.toml" || file.name == "btrc.lock") ./.;
  };
in {
  # The service and its host-side attach helper: no libraries beyond libc.
  deckpad = build {
    name = "deckpad";
    inherit src;
    programs.deckpad = "Main.btrc";
  };
  # Library.Tray binds libdbus-1 for its Linux provider.
  tray = build {
    name = "deckpad-tray";
    inherit src;
    programs.deckpad-tray = "Tray.btrc";
    buildInputs = [ pkgs.dbus ];
    env.NIX_CFLAGS_COMPILE = "-I${pkgs.lib.getDev pkgs.dbus}/include/dbus-1.0 -I${pkgs.lib.getLib pkgs.dbus}/lib/dbus-1.0/include";
    linkFlags = [ "-ldbus-1" ];
  };
}
