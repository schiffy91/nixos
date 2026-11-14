{ inputs ? (builtins.getFlake "git+file://${toString ../../..}").inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  programs = import ../../../config/pkgs/utils/drive/package.nix { inherit pkgs inputs; };
  ask = pkgs.writeShellScriptBin "drive-test-ask" "true";
in (import ../../vm/test.nix { inherit pkgs inputs; }) {
  name = "drive-pinning";
  nodes.machine = { ... }: {
    boot.kernelModules = [ "fuse" ];
    environment.systemPackages = [ programs.drive programs.fake ask pkgs.curl pkgs.attr pkgs.python3 ];
    virtualisation.memorySize = 2048;
  };
  scenario = ./Main.btrc;
  extraFiles = [ ../Fixture.btrc ];
}
