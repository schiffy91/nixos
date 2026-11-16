{ pkgs, inputs }:
(import ../../../lib/btrc.nix { inherit pkgs inputs; }) {
  name = "chrome-launchers";
  src = ./.;
  programs.chrome-launchers = "Main.btrc";
  buildInputs = [ pkgs.glib ];
}
