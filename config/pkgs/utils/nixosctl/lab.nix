# The VM lab: `vm`, `e2e` and `graph`, which `nixosctl` hands to this program. A checkout builds its own
# beside build/nixosctl; this is the installed one, for a machine that runs the scenario graph.
{ pkgs, lib, btrcpy }:
pkgs.stdenv.mkDerivation {
  name = "nixosctl-lab";
  src = lib.fileset.toSource {
    root = ../../../..;
    fileset = lib.fileset.unions [ ./lib ./btrc.toml ./btrc.lock ];
  };
  nativeBuildInputs = [ btrcpy ];
  buildInputs = [ pkgs.libxcrypt ];
  CPATH = lib.makeSearchPath "include" (map lib.getDev [ pkgs.libxcrypt ]);  # the package's header reader parses password-hash.h, which needs crypt.h
  dontConfigure = true;
  buildPhase = ''
    export BTRC_CACHE_DIR="$TMPDIR/btrc-cache"
    export BTRC_STATE_DIR="$TMPDIR/btrc-state"
    export BTRC_PKG_CACHE="$TMPDIR/btrc-pkgs"
    btrcpy --strict-imports config/pkgs/utils/nixosctl/lib/simulator/Main.btrc -o nixosctl-lab.c
    $CC -std=c11 -O2 nixosctl-lab.c -lm -lpthread -lutil -lcrypt -o nixosctl-lab
  '';
  installPhase = ''
    install -Dm755 nixosctl-lab $out/bin/nixosctl-lab
  '';
}
