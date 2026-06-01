{ pkgs, config, lib, inputs, ... }:
let
  system = pkgs.stdenv.hostPlatform.system;
  btrcpy = inputs.btrc.packages.${system}.btrcpy;
  usePrebuilt = config.settings.btrc.prebuiltStdlib.enable;

  archive = pkgs.stdenv.mkDerivation {
    name = "btrc-stdlib";
    dontUnpack = true;
    nativeBuildInputs = [ btrcpy ];
    buildPhase = ''
      export HOME=$TMPDIR
      btrcpy --build-stdlib "$PWD/stdlib"
      $CC -std=c11 -O2 -ffunction-sections -fdata-sections \
        -c "$PWD/stdlib/btrc_stdlib.c" -o btrc_stdlib.o
      ar rcs libbtrc.a btrc_stdlib.o
    '';
    installPhase = ''
      mkdir -p $out
      cp "$PWD/stdlib/btrc_stdlib.h" $out/btrc_stdlib.h
      cp "$PWD/stdlib/btrc_stdlib.c" $out/btrc_stdlib.c
      cp "$PWD/stdlib/btrc_stdlib.manifest" $out/btrc_stdlib.manifest
      cp libbtrc.a $out/libbtrc.a
    '';
  };

  btrcStdlib = rec {
    inherit archive btrcpy usePrebuilt;
    incDir = "${archive}";
    stdlibInput = if usePrebuilt then "${archive}/libbtrc.a" else "${archive}/btrc_stdlib.c";
    gcFlags = "-ffunction-sections -fdata-sections -Wl,--gc-sections";
  };

  shellWords = words: lib.concatStringsSep " " words;

  buildBtrcProgram =
    { name
    , entry
    , nativeBuildInputs ? []
    , buildInputs ? []
    , extraCInputs ? []
    , extraCFlags ? []
    , extraLibs ? []
    , extraInstall ? ""
    }:
    pkgs.stdenv.mkDerivation {
      inherit name buildInputs;
      src = ../../.;
      nativeBuildInputs = [ btrcpy ] ++ nativeBuildInputs;
      buildPhase = ''
        export HOME=$TMPDIR
        btrcpy --no-stdlib --strict-imports --stdlib ${archive} "$src/${entry}" -o ${name}.c
        $CC -std=c11 -O2 -I${btrcStdlib.incDir} ${btrcStdlib.gcFlags} ${shellWords extraCFlags} \
          ${name}.c ${shellWords extraCInputs} ${btrcStdlib.stdlibInput} ${shellWords extraLibs} -o ${name}
      '';
      installPhase = ''
        mkdir -p $out/bin
        cp ${name} $out/bin/${name}
        ${extraInstall}
      '';
    };
in {
  _module.args.btrcStdlib = btrcStdlib;
  _module.args.buildBtrcProgram = buildBtrcProgram;
}
