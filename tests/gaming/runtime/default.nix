{ inputs ? (builtins.getFlake "git+file://${toString ../../..}").inputs
, system ? builtins.currentSystem
, pkgs ? import inputs.nixpkgs { inherit system; }
}:
let
  build = import ../../../config/lib/btrc.nix { inherit pkgs inputs; };
  tools = build {
    name = "gaming-runtime-test-tools";
    src = pkgs.lib.fileset.toSource {
      root = ../../..;
      fileset = pkgs.lib.fileset.unions [
        ../../../config/pkgs/gaming/sunshine/credentials/src
        ../../../config/pkgs/gaming/sunshine/virtual-display
        ../../../config/pkgs/gaming/slopsmith/src
        ../../../config/lib/BackgroundProcess.btrc
        ./Contracts.btrc
      ];
    };
    programs = {
      contracts = "tests/gaming/runtime/Contracts.btrc";
      credentials = "config/pkgs/gaming/sunshine/credentials/src/Main.btrc";
      virtual-display = "config/pkgs/gaming/sunshine/virtual-display/Main.btrc";
      slopsmith-runtime = "config/pkgs/gaming/slopsmith/src/Main.btrc";
    };
  };
  ssh = import ../../../config/pkgs/gaming/sunshine/ssh-login/package.nix { inherit pkgs inputs; };
  icon = pkgs.runCommand "artwork-test-icon.png" { nativeBuildInputs = [ pkgs.imagemagick ]; } ''
    magick -size 16x16 xc:cyan "$out"
  '';
  artwork = import ../../../config/pkgs/gaming/steam/artwork/package.nix { inherit pkgs icon; name = "A & <B> \"C\" 'D'"; };
in pkgs.runCommand "gaming-runtime-contracts" {
  nativeBuildInputs = [ pkgs.python3 pkgs.openssl pkgs.openssh pkgs.coreutils pkgs.bash pkgs.curl ];
} ''
  ${tools}/bin/contracts
  python3 ${./test.py} ${tools}/bin ${ssh}/bin/sunshine-ssh-login ${artwork}
  touch "$out"
''
