{ pkgs, inputs }:
let
  artwork = import ../../config/pkgs/gaming/artwork/package.nix { inherit pkgs inputs; };
  packages = pkgs.linkFarm "gaming-artwork" (pkgs.lib.mapAttrsToList (name: assets: {
    inherit name;
    path = assets.artwork;
  }) artwork);
  tests = (inputs.btrc.lib.buildPackageWith {
    inherit pkgs;
    btrcpy = import ../lib/image-compiler.nix { inherit pkgs inputs; };
    btrc-native-plan = null;
  }) {
    name = "gaming-artwork-tests";
    src = (import ../lib/test-source.nix { inherit pkgs inputs; }) {
      root = ../.;
      fileset = pkgs.lib.fileset.unions [ ./ArtworkTests.btrc ];
    };
    programs.artwork-tests = "gaming/ArtworkTests.btrc";
    buildInputs = [ pkgs.libpng pkgs.libjpeg_turbo ];
    linkFlags = [ "-lpng" "-lturbojpeg" ];
  };
in pkgs.runCommand "gaming-artwork-check" {} ''
  ${tests}/bin/artwork-tests ${packages}
  touch "$out"
''
