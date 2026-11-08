{ pkgs, inputs }:
import ../../../lib/btrc.nix { inherit pkgs inputs; } {
  name = "trusted-connections";
  src = ./.;
  programs.trusted-connections = "Main.btrc";
  runtimeInputs = [ pkgs.iproute2 pkgs.iptables pkgs.networkmanager pkgs.systemd ];
}
