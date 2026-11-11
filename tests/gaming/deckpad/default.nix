{ pkgs, inputs }:
let
  programs = import ../../../config/pkgs/gaming/deckpad/package.nix { inherit pkgs inputs; };
  tests = (import ../../../config/lib/btrc.nix { inherit pkgs inputs; }) {
    name = "deckpad-tests";
    src = pkgs.lib.fileset.toSource {
      root = ../../..;
      fileset = pkgs.lib.fileset.unions [
        ./UnitTests.btrc ./ServerTests.btrc ./btrc.toml ./btrc.lock ../../lib/Check.btrc
        ../../../config/lib/BackgroundProcess.btrc
        (pkgs.lib.fileset.fileFilter (file: file.hasExt "btrc" || file.hasExt "h" || file.name == "btrc.toml" || file.name == "btrc.lock") ../../../config/pkgs/gaming/deckpad)
      ];
    };
    programs = {
      deckpad-unit-tests = "tests/gaming/deckpad/UnitTests.btrc";
      deckpad-server-tests = "tests/gaming/deckpad/ServerTests.btrc";
    };
  };
# The server test plays the USB/IP host against the real program over loopback.
in pkgs.runCommand "deckpad-check" {} ''
  ${tests}/bin/deckpad-unit-tests
  ${tests}/bin/deckpad-server-tests ${programs.deckpad}/bin/deckpad
  ${programs.tray}/bin/deckpad-tray
  touch "$out"
''
