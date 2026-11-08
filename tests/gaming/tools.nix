{ pkgs, inputs }:
let tools = import ../../config/pkgs/gaming/package.nix { inherit pkgs inputs; };
in pkgs.stdenv.mkDerivation {
  name = "gaming-tools-tests";
  src = pkgs.lib.fileset.toSource {
    root = ../..;
    fileset = pkgs.lib.fileset.unions [
      ../../config/pkgs/gaming/assettocorsa/src
      ../../config/pkgs/gaming/steam/src
      ../../config/pkgs/gaming/wine/src
      ../../config/pkgs/gaming/lib/GamingFiles.btrc
      ./GamingTests.btrc
      ./btrc.toml
    ];
  };
  nativeBuildInputs = [ inputs.btrc.packages.${pkgs.stdenv.hostPlatform.system}.btrcpy pkgs.pkg-config ];
  buildInputs = [ pkgs.liblzf pkgs.zlib ];
  CPATH = "${pkgs.liblzf.dev}/include";
  dontConfigure = true;
  buildPhase = ''
    export BTRC_CACHE_DIR="$TMPDIR/btrc-cache"
    export BTRC_STATE_DIR="$TMPDIR/btrc-state"
    export BTRC_PKG_CACHE="$TMPDIR/btrc-packages"
    btrcpy --strict-imports tests/gaming/GamingTests.btrc -o gaming-tests.c
    $CC -std=c11 -O2 gaming-tests.c -lm -lpthread -llzf -lz -o gaming-tests
  '';
  doCheck = true;
  nativeCheckInputs = [ pkgs.coreutils pkgs.procps pkgs.libarchive ];
  checkPhase = ''
    export GAMING_TOOLS=${tools}/bin
    ./gaming-tests steam
    ./gaming-tests wine
    ./gaming-tests assetto
    ./gaming-tests integration
  '';
  installPhase = ''
    mkdir -p "$out"
    touch "$out/passed"
  '';
}
