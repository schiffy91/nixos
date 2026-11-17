{ pkgs, inputs }:
(inputs.btrc.lib.buildPackage pkgs) {
  name = "sunshine-ssh-login";
  src = ./.;
  programs.sunshine-ssh-login = "Main.btrc";
  buildInputs = [ pkgs.curl pkgs.openssl ];
  runtimeInputs = [ pkgs.openssh ];
}
