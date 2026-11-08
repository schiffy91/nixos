{ flake ? builtins.getFlake "git+file://${toString ../../..}"
, inputs ? flake.inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  contracts = (import ../../../config/lib/btrc.nix { inherit pkgs inputs; }) {
    name = "sunshine-credentials-contracts";
    src = pkgs.lib.fileset.toSource {
      root = ../../..;
      fileset = pkgs.lib.fileset.unions [ ../../lib ../../../config/lib/BackgroundProcess.btrc ./GuestTests.btrc ./Fixture.btrc ];
    };
    programs.sunshine-credentials-contracts = "tests/nixosctl/sunshine/GuestTests.btrc";
  };
in (import ../../vm/test.nix { inherit pkgs inputs; }) {
  name = "sunshine-credentials";
  node.pkgsReadOnly = false;
  nodes.machine = { config, lib, pkgs, ... }: {
    _module.args.inputs = inputs;
    imports = [
      (inputs.nixpkgs + "/nixos/tests/common/x11.nix")
      ../../../config/settings.nix
      ../../../config/pkgs/gaming/sunshine/default.nix
    ];
    settings.users.admin = { username = "operator"; homeDirectory = "/srv/operator"; };
    users.users.operator = { isNormalUser = true; home = "/srv/operator"; uid = 1000; extraGroups = [ "video" "audio" "input" ]; };
    test-support.displayManager.auto.user = "operator";
    services.sunshine = {
      autoStart = lib.mkForce false;
      capSysAdmin = lib.mkForce false;
      settings = {
        capture = lib.mkForce "x11";
        encoder = "software";
        system_tray = "disabled";
      };
    };
    systemd.user.services.sunshine.environment = { DISPLAY = ":0"; XAUTHORITY = "/srv/operator/.Xauthority"; };
    environment.systemPackages = [
      contracts pkgs.curl pkgs.openssl pkgs.netcat-openbsd pkgs.moonlight-qt
      (import ../../../config/pkgs/utils/nixosctl/package.nix {
        inherit pkgs lib;
        btrcpy = inputs.btrc.packages.${system}.btrcpy;
      })
    ];
    virtualisation.memorySize = 4096;
    virtualisation.cores = 4;
  };
  scenario = ./Main.btrc;
}
