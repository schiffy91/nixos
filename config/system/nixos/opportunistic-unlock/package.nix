{ pkgs, inputs }:
inputs.btrc.lib.buildPackage pkgs {
  name = "nixos-opportunistic-unlock";
  src = ./.;
  programs.nixos-opportunistic-unlock = "Main.btrc";
  buildInputs = [ pkgs.cryptsetup ];
}
