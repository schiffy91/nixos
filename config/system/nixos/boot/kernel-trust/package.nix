{ pkgs, inputs }:
(import ../../../../lib/btrc.nix { inherit pkgs inputs; }) {
  name = "check-module-trust";
  src = pkgs.lib.fileset.toSource {
    root = ./.;
    fileset = pkgs.lib.fileset.unions [ ./Main.btrc ./Modules.btrc ];
  };
  programs.check-module-trust = "Main.btrc";
  runtimeInputs = with pkgs; [ openssl gzip xz zstd ];
}
