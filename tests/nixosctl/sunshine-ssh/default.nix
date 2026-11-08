{ flake ? builtins.getFlake "git+file://${toString ../../..}"
, inputs ? flake.inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  contracts = (import ../../../config/lib/btrc.nix { inherit pkgs inputs; }) {
    name = "sunshine-ssh-contracts";
    src = pkgs.lib.fileset.toSource {
      root = ../../..;
      fileset = pkgs.lib.fileset.unions [ ../../lib ../../../config/lib/BackgroundProcess.btrc ../sunshine/Fixture.btrc ./GuestTests.btrc ];
    };
    programs.sunshine-ssh-contracts = "tests/nixosctl/sunshine-ssh/GuestTests.btrc";
  };
in (import ../../vm/test.nix { inherit pkgs inputs; }) {
  name = "sunshine-ssh-login";
  node.pkgsReadOnly = false;
  nodes.machine = { config, lib, pkgs, ... }: {
    _module.args.inputs = inputs;
    imports = [
      (inputs.nixpkgs + "/nixos/tests/common/x11.nix")
      ../../../config/settings.nix
      ../../../config/pkgs/gaming/sunshine/default.nix
    ];
    settings.users.admin = { username = "operator"; homeDirectory = "/srv/operator"; };
    settings.apps.gaming.sunshine.sshLogin = {
      enable = true;
      publicKeys = [ (lib.removeSuffix "\n" (builtins.readFile ./fixture-key.pub)) ];
    };
    users.users.operator = { isNormalUser = true; home = "/srv/operator"; uid = 1000; extraGroups = [ "video" "audio" "input" ]; };
    networking.hosts."127.0.0.1" = [ "sunshine.test" ];
    test-support.displayManager.auto.user = "operator";
    services.sunshine = {
      autoStart = lib.mkForce false;
      capSysAdmin = lib.mkForce false;
      settings = {
        capture = lib.mkForce "x11";
        encoder = "software";
        system_tray = "disabled";
        cert = "/srv/operator/.config/sunshine/server.crt";
        pkey = "/srv/operator/.config/sunshine/server.key";
      };
    };
    systemd.user.services.sunshine.environment = { DISPLAY = ":0"; XAUTHORITY = "/srv/operator/.Xauthority"; };
    environment.systemPackages = [ contracts pkgs.curl pkgs.openssh ];
    environment.etc."sunshine-ssh-fixture".source = ./fixture-key;
    environment.etc."sunshine-ssh-fixture.pub".source = ./fixture-key.pub;
    virtualisation.memorySize = 3072;
    virtualisation.cores = 2;
  };
  scenario = ./Main.btrc;
}
