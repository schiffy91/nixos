{ pkgs, btrcpy }:
(import ../../../../lib/btrc.nix { inherit pkgs btrcpy; }) {
  name = "nixosctl-secure-boot-keys";
  src = pkgs.lib.fileset.toSource {
    root = ./.;
    fileset = pkgs.lib.fileset.unions [ ./Main.btrc ./Archive.btrc ./Files.btrc ./Crypto.btrc ./btrc.toml ./crypto.h ./files.h ];
  };
  programs.nixosctl-secure-boot-keys = "Main.btrc";
  buildInputs = [ pkgs.openssl ];
  linkFlags = [ "-lcrypto" ];
}
