{ pkgs, inputs }:
import ../../../lib/btrc.nix { inherit pkgs inputs; } {
  name = "nixos-opportunistic-unlock";
  src = ./.;
  programs.nixos-opportunistic-unlock = "Main.btrc";
  buildInputs = [ pkgs.cryptsetup ];
  linkFlags = [ "-lcryptsetup" ];
}
