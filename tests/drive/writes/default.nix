{ inputs ? (builtins.getFlake "git+file://${toString ../../..}").inputs
, system ? builtins.currentSystem
, mutation ? ""  # a change to the client's source that takes one rule away; the scenario must then fail
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  programs = import ../../../config/pkgs/utils/drive/package.nix { inherit pkgs inputs; };
  drive = if mutation == "" then programs.drive else programs.drive.overrideAttrs (_: { postPatch = mutation; });
  ask = pkgs.writeShellScriptBin "drive-test-ask" "true";
in (import ../../vm/test.nix { inherit pkgs inputs; }) {
  name = "drive-writes";
  nodes.machine = { ... }: {
    boot.kernelModules = [ "fuse" ];
    environment.systemPackages = [ drive programs.fake ask pkgs.curl pkgs.attr pkgs.python3 ];
    virtualisation.memorySize = 2048;
  };
  scenario = ./Main.btrc;
  settings = pkgs.lib.optionalAttrs (mutation != "") { patienceSeconds = 30; };  # a rule cut out shows as a wait that never ends
  extraFiles = [ ../Fixture.btrc ];
}
