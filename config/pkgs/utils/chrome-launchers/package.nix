{ pkgs, inputs }:
(inputs.btrc.lib.buildPackage pkgs) {
  name = "chrome-launchers";
  src = ./.;
  programs.chrome-launchers = "Main.btrc";
  buildInputs = [ pkgs.glib ];
}
