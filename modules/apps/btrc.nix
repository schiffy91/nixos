{ pkgs, config, lib, ... }:
# Precompiled BTRC standard library, exposed to other modules as the
# `btrcStdlib` module argument (this file is auto-imported as a module by
# modules/system/admin.nix's `nixFiles ../apps`).
#
# `btrcpy --build-stdlib` (see the Makefile) compiles the whole stdlib once into
# generated/btrc_stdlib.{c,h} + a manifest. Here we compile that checked-in C
# into a single static archive (libbtrc.a). The membrane, nixosctl, and tray
# link this archive instead of each re-compiling the entire stdlib inlined into
# their own translation unit (their generated/*.c are program-only and
# `#include "btrc_stdlib.h"`).
#
# Static linking keeps every binary self-contained — in particular the membrane,
# which runs in the initrd before sysroot.mount, needs nothing extra placed in
# the initrd (the stdlib is baked into the binary, exactly as before).
let
  gen = ../../generated;
  usePrebuilt = config.settings.btrc.prebuiltStdlib.enable;

  archive = pkgs.stdenv.mkDerivation {
    name = "btrc-stdlib";
    dontUnpack = true;
    # -ffunction-sections/-fdata-sections so consumers can drop unused stdlib
    # with -Wl,--gc-sections and stay lean.
    buildPhase = ''
      $CC -std=c11 -O2 -ffunction-sections -fdata-sections \
        -c ${gen}/btrc_stdlib.c -o btrc_stdlib.o
      ar rcs libbtrc.a btrc_stdlib.o
    '';
    installPhase = ''
      mkdir -p $out
      cp libbtrc.a $out/libbtrc.a
      cp ${gen}/btrc_stdlib.h $out/btrc_stdlib.h
    '';
  };
in {
  _module.args.btrcStdlib = {
    inherit archive usePrebuilt;

    # Spliced into each consumer's $CC line:
    #   -I${incDir}      finds btrc_stdlib.h
    #   ${stdlibInput}   the code to link (prebuilt archive, or the stdlib source
    #                    compiled straight in when the prebuilt path is disabled —
    #                    same static result, just not shared across binaries)
    incDir = if usePrebuilt then "${archive}" else "${gen}";
    stdlibInput = if usePrebuilt then "${archive}/libbtrc.a" else "${gen}/btrc_stdlib.c";
    gcFlags = "-ffunction-sections -fdata-sections -Wl,--gc-sections";
  };
}
