{ pkgs, inputs }:
pkgs.stdenv.mkDerivation {
  name = "gaming-tools";
  src = pkgs.lib.fileset.toSource {
    root = ../../..;
    fileset = pkgs.lib.fileset.unions [
      ../../../btrc/gaming
      ../../../tests/unit/gaming.btrc
      ../../../tests/unit/btrc.toml
    ];
  };
  nativeBuildInputs = [ inputs.btrc.packages.${pkgs.stdenv.hostPlatform.system}.btrcpy pkgs.makeWrapper pkgs.pkg-config ];
  buildInputs = [ pkgs.liblzf pkgs.zlib ];
  # liblzf ships no pkg-config file; Clang's typed header reader needs its headers too.
  CPATH = "${pkgs.liblzf.dev}/include";
  dontConfigure = true;
  buildPhase = ''
    runHook preBuild
    build_tool() {
      btrcpy --strict-imports "$1" -o "$2.c"
      $CC -std=c11 -O2 "$2.c" -lm -lpthread -llzf -lz -o "$2"
    }
    build_tool btrc/gaming/steam/configure.btrc configure-steam-apps
    build_tool btrc/gaming/wine/dpi.btrc wine-prefix-dpi
    build_tool btrc/gaming/assettocorsa/assetto.btrc assetto
    build_tool tests/unit/gaming.btrc gaming-tests
    runHook postBuild
  '';
  doCheck = true;
  nativeCheckInputs = [ pkgs.coreutils pkgs.procps ];
  checkPhase = ''
    runHook preCheck
    export GAMING_TOOLS="$PWD"
    ./gaming-tests
    runHook postCheck
  '';
  installPhase = ''
    mkdir -p "$out/bin"
    for tool in configure-steam-apps wine-prefix-dpi assetto; do
      install -m755 "$tool" "$out/bin/$tool"
      wrapProgram "$out/bin/$tool" --prefix PATH : ${pkgs.lib.makeBinPath [ pkgs.coreutils pkgs.procps ]}
    done
  '';
}
