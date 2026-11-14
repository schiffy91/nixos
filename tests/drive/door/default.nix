{ inputs ? (builtins.getFlake "git+file://${toString ../../..}").inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  programs = import ../../../config/pkgs/utils/drive/package.nix { inherit pkgs inputs; };
in (import ../../vm/test.nix { inherit pkgs inputs; }) {
  name = "drive-door";
  nodes.machine = { ... }: {
    boot.kernelModules = [ "fuse" ];
    environment.systemPackages = [ programs.drive programs.fake pkgs.curl pkgs.attr ];
    virtualisation.memorySize = 2048;
  };
  scenario = ./Main.btrc;
}
