{ inputs ? (builtins.getFlake "git+file://${toString ../../..}").inputs
, system ? builtins.currentSystem
, pkgs ? import inputs.nixpkgs { inherit system; }
}:
import ../../../config/lib/btrc.nix { inherit pkgs inputs; } {
  name = "sleep-helper-contracts";
  src = pkgs.lib.fileset.toSource {
    root = ../../..;
    fileset = pkgs.lib.fileset.unions [
      ./btrc.toml ./HelperTests.btrc ./Prompt.btrc
      ../../../config/system/nixos/sleep/helpers
    ];
  };
  programs = {
    helper-tests = "tests/sleep-lock/helpers/HelperTests.btrc";
    prompt = "tests/sleep-lock/helpers/Prompt.btrc";
    authenticate = "config/system/nixos/sleep/helpers/Authenticate.btrc";
    blank = "config/system/nixos/sleep/helpers/Blank.btrc";
  };
  buildInputs = [ pkgs.pam pkgs.libdrm ];
  linkFlags = [ "-D_GNU_SOURCE" "-lpam" "-ldrm" "-I${pkgs.lib.getDev pkgs.libdrm}/include/libdrm" ];
  postInstall = ''
    $CC -std=c11 -O2 -D_GNU_SOURCE helper-tests.c -I${pkgs.lib.getDev pkgs.libdrm}/include/libdrm -lpam -ldrm -Wl,--wrap=explicit_bzero,--defsym=__wrap_explicit_bzero=testWipe,--wrap=__explicit_bzero_chk,--defsym=__wrap___explicit_bzero_chk=testCheckedWipe -o wrapped-tests
    ./wrapped-tests "$PWD/prompt"
    if grep -Eq '__btrc_(string|arc|cleanup|throw)|pthread_create|pthread_mutex' authenticate.c; then
      echo 'managed or threaded runtime in password helper' >&2
      exit 1
    fi
    grep -q 'mlockall' authenticate.c
    grep -q 'PR_SET_PDEATHSIG' authenticate.c
    nm -D authenticate | grep -q explicit_bzero
    if ./authenticate; then exit 1; fi
  '';
}
