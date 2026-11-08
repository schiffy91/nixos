{ flake ? builtins.getFlake "git+file://${toString ../..}", inputs ? flake.inputs, system ? builtins.currentSystem }:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  tests = (import ../../config/lib/btrc.nix { inherit pkgs inputs; }) {
    name = "recovery-mount-tests";
    src = pkgs.lib.fileset.toSource {
      root = ../..;
      fileset = pkgs.lib.fileset.unions [ ../../config/system/nixos/recovery/Mount.btrc ./MountTests.btrc ./Driver.btrc ];
    };
    programs = {
      recovery-mount-tests = "tests/recovery/MountTests.btrc";
      recovery-mount-driver = "tests/recovery/Driver.btrc";
    };
  };
in pkgs.runCommand "recovery-mount-contracts" {
  nativeBuildInputs = [ pkgs.python3 pkgs.coreutils pkgs.bash ];
} ''
  ${tests}/bin/recovery-mount-tests
  python3 ${./mount.py} ${tests}/bin/recovery-mount-driver ${./commands.py}
  touch "$out"
''
