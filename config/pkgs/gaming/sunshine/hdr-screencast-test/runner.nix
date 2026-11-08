{ system ? builtins.currentSystem
, inputs ? (builtins.getFlake "git+file://${toString ../../../../..}").inputs
, pkgs ? import inputs.nixpkgs { inherit system; }
}:
let probe = import ./package.nix { inherit pkgs; };
in (import ../../../../lib/btrc.nix { inherit pkgs inputs; }) {
  name = "hdr-screencast-test";
  src = pkgs.lib.fileset.toSource { root = ./.; fileset = pkgs.lib.fileset.unions [ ./Main.btrc ./Capture.btrc ]; };
  programs.hdr-screencast-test = "Main.btrc";
  runtimeInputs = [ pkgs.dbus ];
  wrapperArgs = [ "--set" "HDR_SCREENCAST_PROBE" "${probe}/bin/hdr-screencast-probe" ];
}
