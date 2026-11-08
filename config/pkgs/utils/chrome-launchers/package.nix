{ pkgs, inputs }:
((import ../../../lib/btrc.nix { inherit pkgs inputs; }) {
  name = "chrome-launchers";
  src = ./.;
  programs.chrome-launchers = "Main.btrc";
  buildInputs = [ pkgs.glib ];
  linkFlags = [ "-lglib-2.0" ];
}).overrideAttrs {
  preBuild = ''
    export NIX_CFLAGS_COMPILE="$NIX_CFLAGS_COMPILE $(pkg-config --cflags glib-2.0)"
  '';
}
