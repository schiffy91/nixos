{ system ? builtins.currentSystem }:
let
  locked = (builtins.fromJSON (builtins.readFile ../../../flake.lock)).nodes.nixpkgs.locked;
  pkgs = import (builtins.fetchTree { inherit (locked) type owner repo rev narHash; }) { inherit system; };
  btrc = (builtins.fromJSON (builtins.readFile ../../../flake.lock)).nodes.btrc.locked;
  btrcFlake = builtins.getFlake (builtins.flakeRefToString (builtins.removeAttrs btrc [
    "narHash" "lastModified" "lastModifiedDate" "revCount" "dirtyRev" "dirtyShortRev"
  ]));
  inputs.btrc = btrcFlake;
  listener = (inputs.btrc.lib.buildPackage pkgs) {
    name = "network-test-listener";
    src = pkgs.lib.fileset.toSource { root = ./.; fileset = ./Listener.btrc; };
    programs.network-test-listener = "Listener.btrc";
  };
in (import ../../lib/vm-test.nix { inherit pkgs inputs; }) {
  name = "trusted-network-connections";
  nodes.machine = { lib, pkgs, inputs, ... }: {
    imports = [ ../../../config/settings.nix ../../../config/system/nixos/networking.nix ];
    _module.args.inputs.btrc = btrcFlake;
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
      networkmanager.unmanaged = lib.mkForce [ "interface-name:eth0" ];
    };
    systemd.services = builtins.listToAttrs (map (port: {
      name = "http-${toString port}";
      value = {
        wantedBy = [ "multi-user.target" ];
        serviceConfig.ExecStart = "${listener}/bin/network-test-listener ${toString port}";
      };
    }) [ 8080 8081 ]);
    environment.systemPackages = [ pkgs.iptables (import ../../../config/system/nixos/networking/package.nix { inherit pkgs inputs; }) ];
  };
  nodes.client = { lib, pkgs, ... }: {
    networking.interfaces.eth1.ipv4.addresses = lib.mkForce [{ address = "10.55.1.2"; prefixLength = 24; }];
    environment.systemPackages = [ pkgs.netcat-openbsd ];
  };
  scenario = ./Main.btrc;
}
