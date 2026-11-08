{ system ? builtins.currentSystem }:
let
  locked = (builtins.fromJSON (builtins.readFile ../../../flake.lock)).nodes.nixpkgs.locked;
  pkgs = import (builtins.fetchTree { inherit (locked) type owner repo rev narHash; }) { inherit system; };
in pkgs.testers.runNixOSTest {
  name = "trusted-network-connections";
  nodes.machine = { lib, pkgs, ... }: {
    imports = [ ../../../config/settings.nix ../../../config/system/nixos/networking.nix ];
    _module.args.inputs.btrc = let btrc = (builtins.fromJSON (builtins.readFile ../../../flake.lock)).nodes.btrc.locked;
      in builtins.getFlake "github:${btrc.owner}/${btrc.repo}/${btrc.rev}";
    settings.networking = {
      trustedConnections = [ "11111111-1111-4111-8111-111111111111" ];
      ports.tcp = [ 8080 ];
    };
    networking = {
      useDHCP = false;
      interfaces.eth1 = {
        useDHCP = lib.mkForce false;
        ipv4.addresses = lib.mkForce [];
      };
      networkmanager.unmanaged = lib.mkForce [];
    };
    systemd.services = builtins.listToAttrs (map (port: {
      name = "http-${toString port}";
      value = {
        wantedBy = [ "multi-user.target" ];
        serviceConfig.ExecStart = "${pkgs.python3}/bin/python3 -m http.server ${toString port} --bind 0.0.0.0";
      };
    }) [ 8080 8081 ]);
    environment.systemPackages = [ pkgs.iptables ];
  };
  nodes.client = { lib, pkgs, ... }: {
    networking.interfaces.eth1.ipv4.addresses = lib.mkForce [{ address = "10.55.1.2"; prefixLength = 24; }];
    environment.systemPackages = [ pkgs.netcat-openbsd ];
  };
  testScript = builtins.readFile ./test.py;
}
