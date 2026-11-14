{ inputs ? (builtins.getFlake "git+file://${toString ../../..}").inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  programs = import ../../../config/pkgs/utils/drive/package.nix { inherit pkgs inputs; };
in (import ../../vm/test.nix { inherit pkgs inputs; }) {
  name = "drive-convergence";
  nodes.machine = { ... }: {
    boot.kernelModules = [ "fuse" ];
    environment.systemPackages = [ programs.drive programs.fake pkgs.curl pkgs.attr pkgs.python3 ];
    environment.etc."drive-test/operations.py".source = ./operations.py;
    environment.etc."drive-test/tree.py".source = ./tree.py;
    virtualisation.memorySize = 2048;
  };
  scenario = ./Main.btrc;
  extraFiles = [ ../Fixture.btrc ];
}
