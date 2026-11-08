{ flake ? builtins.getFlake "git+file://${toString ../..}", inputs ? flake.inputs, system ? builtins.currentSystem }:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  tests = (import ../../config/lib/btrc.nix { inherit pkgs inputs; }) {
    name = "recovery-mount-tests";
    src = pkgs.lib.fileset.toSource {
      root = ../..;
      fileset = pkgs.lib.fileset.unions [ ../../config/system/nixos/recovery/Main.btrc ../../config/system/nixos/recovery/Mount.btrc ../lib ./MountTests.btrc ./Driver.btrc ./MountFixture.btrc ./MountIntegration.btrc ];
    };
    programs = {
      recovery-mount-tests = "tests/recovery/MountTests.btrc";
      recovery-mount-driver = "tests/recovery/Driver.btrc";
      recovery-mount-command = "config/system/nixos/recovery/Main.btrc";
      recovery-mount-fixture = "tests/recovery/MountFixture.btrc";
      recovery-mount-integration = "tests/recovery/MountIntegration.btrc";
    };
  };
in pkgs.runCommand "recovery-mount-contracts" {
  nativeBuildInputs = [ pkgs.coreutils pkgs.bash ];
} ''
  ${tests}/bin/recovery-mount-tests
  ${tests}/bin/recovery-mount-integration ${tests}/bin/recovery-mount-driver ${tests}/bin/recovery-mount-fixture
  ${tests}/bin/recovery-mount-command absent-plan --help | grep -F 'Usage: nixos-mount [target]'
  if ${tests}/bin/recovery-mount-command >invalid.log 2>&1; then
    echo "Missing recovery plan was accepted" >&2; exit 1
  fi
  grep -F 'Usage: nixos-mount [target]' invalid.log
  touch "$out"
''
