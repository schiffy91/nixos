{ pkgs, inputs }:
inputs.btrc.lib.buildPackage pkgs {
  name = "trusted-connections";
  src = ./.;
  programs.trusted-connections = "Main.btrc";
  runtimeInputs = [ pkgs.iproute2 pkgs.iptables pkgs.networkmanager pkgs.systemd ];
}
