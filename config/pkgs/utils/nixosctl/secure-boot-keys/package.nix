{ pkgs, inputs, btrcpy }:
(inputs.btrc.lib.buildPackageWith { inherit pkgs btrcpy; btrc-native-plan = null; }) {
  name = "nixosctl-secure-boot-keys";
  src = pkgs.lib.fileset.toSource {
    root = ./.;
    fileset = pkgs.lib.fileset.unions [ ./Main.btrc ./Archive.btrc ./Files.btrc ./Crypto.btrc ./btrc.toml ./btrc.lock ./crypto.h ./files.h ];
  };
  programs.nixosctl-secure-boot-keys = "Main.btrc";
  buildInputs = [ pkgs.openssl ];
  linkFlags = [ "-lcrypto" ];
}
