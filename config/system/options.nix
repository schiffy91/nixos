{ lib, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  imports = [
    ./nixos/users/options.nix
    ./storage/options.nix
    ./storage/immutability/options.nix
    ./nixos/boot/options.nix
    ./nixos/graphics/options.nix
    ./nixos/sleep/options.nix
    ./nixos/networking/options.nix
    ./darwin/options.nix
  ];
  options = with lib.types; {
    settings.secrets.path = mkSetting str "/etc/nixos/secrets";
    settings.nix.substituters = mkSetting (listOf str) []; # extra binary caches, tried in order
    settings.nix.trustedPublicKeys = mkSetting (listOf str) []; # the keys those caches sign with
    settings.input.libinputMice = mkSetting (listOf (submodule {
      options = {
        vendorId = mkSetting str null;
        productId = mkSetting str null;
        name = mkSetting str null;
        accelProfile = mkSetting (enum [ "flat" "adaptive" ]) "flat";
      };
    })) [];
  };
}
