{ pkgs, inputs }:
let
  programs = (import ../../../config/lib/btrc.nix { inherit pkgs inputs; }) {
    name = "wow-lock-contract-tests";
    src = pkgs.lib.fileset.toSource {
      root = ../../..;
      fileset = pkgs.lib.fileset.unions [ ../../../config/pkgs/gaming/wow/Lock.btrc ../../../config/pkgs/gaming/wow/Main.btrc ./LockTests.btrc ];
    };
    programs = { wow-addons-lock = "config/pkgs/gaming/wow/Main.btrc"; wow-lock-tests = "tests/gaming/wow/LockTests.btrc"; };
  };
in pkgs.runCommand "wow-lock-contract-check" { nativeBuildInputs = [ pkgs.python3 pkgs.coreutils ]; } ''
  ${programs}/bin/wow-lock-tests
  python3 ${./test.py} ${programs}/bin/wow-addons-lock
  touch "$out"
''
