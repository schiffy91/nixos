{ inputs ? (builtins.getFlake "git+file://${toString ../../..}").inputs
, system ? builtins.currentSystem
, mutation ? ""  # a change to the client's source that takes one rule away; the scenario must then fail
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  programs = import ../../../config/pkgs/utils/drive/package.nix { inherit pkgs inputs; };
  drive = if mutation == "" then programs.drive else programs.drive.overrideAttrs (_: { postPatch = mutation; });
in (import ../../lib/vm-test.nix { inherit pkgs inputs; }) {
  name = "drive-held";
  nodes.machine = { ... }: {
    boot.kernelModules = [ "fuse" ];
    environment.systemPackages = [ drive pkgs.curl pkgs.attr pkgs.python3 ];
    virtualisation.memorySize = 2048;
  };
  # The Drive is a machine of its own: the fake keeps what it holds in memory, and the client's machine restarts.
  nodes.cloud = { ... }: {
    networking.firewall.allowedTCPPorts = [ 18475 ];
    systemd.services.drive-fake = {
      wantedBy = [ "multi-user.target" ];
      serviceConfig.ExecStart = "${programs.fake}/bin/drive-fake 18475 --reachable";
    };
  };
  scenario = ./Main.btrc;
  settings = pkgs.lib.optionalAttrs (mutation != "") { patienceSeconds = 30; };  # a rule cut out shows as a wait that never ends
  extraFiles = [ ../Fixture.btrc ];
}
