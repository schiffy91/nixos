{ pkgs, inputs }:
(import ../../../lib/btrc.nix { inherit pkgs inputs; }) {
  name = "wow-addons-lock";
  src = pkgs.lib.fileset.toSource { root = ./.; fileset = pkgs.lib.fileset.unions [ ./Main.btrc ./Lock.btrc ]; };
  programs.wow-addons-lock = "Main.btrc";
  runtimeInputs = [ pkgs.curl pkgs.nix pkgs.git pkgs.coreutils ];
}
