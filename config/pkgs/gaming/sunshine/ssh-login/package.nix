{ pkgs, inputs }:
(import ../../../../lib/btrc.nix { inherit pkgs inputs; }) {
  name = "sunshine-ssh-login";
  src = ./.;
  programs.sunshine-ssh-login = "Main.btrc";
  buildInputs = [ pkgs.curl pkgs.openssl ];
  linkFlags = [ "-lcurl" "-lssl" "-lcrypto" ];
  runtimeInputs = [ pkgs.openssh ];
}
