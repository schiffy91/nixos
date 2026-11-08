{ pkgs, inputs ? null, btrcpy ? inputs.btrc.packages.${pkgs.stdenv.buildPlatform.system}.btrcpy }:
{ name, src, programs, buildInputs ? [], runtimeInputs ? [], linkFlags ? []
, nativeBuildInputs ? [], env ? {}, wrapperArgs ? [], postInstall ? "" }:
pkgs.stdenv.mkDerivation {
  inherit name src buildInputs env postInstall;
  nativeBuildInputs = [ btrcpy pkgs.makeWrapper pkgs.pkg-config ] ++ nativeBuildInputs;
  CPATH = pkgs.lib.makeSearchPath "include" (map pkgs.lib.getDev buildInputs);
  dontConfigure = true;
  buildPhase = ''
    runHook preBuild
    export BTRC_CACHE_DIR="$TMPDIR/btrc-cache"
    export BTRC_STATE_DIR="$TMPDIR/btrc-state"
    export BTRC_PKG_CACHE="$TMPDIR/btrc-packages"
    ${pkgs.lib.concatStringsSep "\n" (pkgs.lib.mapAttrsToList (binary: entry: ''
      btrcpy --strict-imports ${pkgs.lib.escapeShellArg entry} -o ${pkgs.lib.escapeShellArg "${binary}.c"}
      $CC -std=c11 -O2 ${pkgs.lib.escapeShellArg "${binary}.c"} -lm -lpthread ${pkgs.lib.escapeShellArgs linkFlags} -o ${pkgs.lib.escapeShellArg binary}
    '') programs)}
    runHook postBuild
  '';
  installPhase = ''
    runHook preInstall
    mkdir -p "$out/bin"
    ${pkgs.lib.concatMapStringsSep "\n" (binary: ''
      install -m755 ${pkgs.lib.escapeShellArg binary} "$out/bin/${binary}"
      ${pkgs.lib.optionalString (runtimeInputs != [] || wrapperArgs != []) ''
        wrapProgram "$out/bin/${binary}" ${pkgs.lib.escapeShellArgs (
          pkgs.lib.optionals (runtimeInputs != []) [ "--prefix" "PATH" ":" (pkgs.lib.makeBinPath runtimeInputs) ] ++ wrapperArgs
        )}
      ''}
    '') (builtins.attrNames programs)}
    runHook postInstall
  '';
}
