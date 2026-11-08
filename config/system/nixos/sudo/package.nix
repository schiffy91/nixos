{ pkgs, inputs, admin }:
(import ../../../lib/btrc.nix { inherit pkgs inputs; }) {
  name = "nixosctl-sudo";
  src = pkgs.lib.fileset.toSource {
    root = ./.;
    fileset = pkgs.lib.fileset.unions [ ./Main.btrc ./Grant.btrc ];
  };
  programs.nixosctl-sudo = "Main.btrc";
  runtimeInputs = [ pkgs.coreutils ];
  wrapperArgs = [ "--add-flags" (pkgs.lib.escapeShellArgs [ admin "${pkgs.sudo}/sbin/visudo" ]) ];
}
