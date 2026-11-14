{ pkgs, inputs ? null, btrcpy ? inputs.btrc.packages.${pkgs.stdenv.buildPlatform.system}.btrcpy }:
# `linkPlan` builds through btrc's native plan, which a program needs once its imports bring
# their own C sources and pkg-config packages (the GUI); `inputs` must then be given.
{ name, src, programs, buildInputs ? [], runtimeInputs ? [], linkFlags ? [], linkPlan ? false
, nativeBuildInputs ? [], env ? {}, wrapperArgs ? [], postInstall ? "" }:
pkgs.stdenv.mkDerivation {
  inherit name src buildInputs env postInstall;
  nativeBuildInputs = [ btrcpy pkgs.makeWrapper pkgs.pkg-config ] ++ nativeBuildInputs
    ++ pkgs.lib.optional linkPlan inputs.btrc.packages.${pkgs.stdenv.buildPlatform.system}.btrc-native-plan;
  CPATH = pkgs.lib.makeSearchPath "include" (map pkgs.lib.getDev buildInputs);
  dontConfigure = true;
  buildPhase = ''
    runHook preBuild
    export BTRC_CACHE_DIR="$TMPDIR/btrc-cache"
    export BTRC_STATE_DIR="$TMPDIR/btrc-state"
    export BTRC_PKG_CACHE="$TMPDIR/btrc-packages"
    ${pkgs.lib.concatStringsSep "\n" (pkgs.lib.mapAttrsToList (binary: entry: if linkPlan then ''
      btrcpy --strict-imports ${pkgs.lib.escapeShellArg entry} -o ${pkgs.lib.escapeShellArg "${binary}.c"} --emit-link-plan ${pkgs.lib.escapeShellArg "${binary}.link.json"}
      btrc-native-plan --plan ${pkgs.lib.escapeShellArg "${binary}.link.json"} --generated-c ${pkgs.lib.escapeShellArg "${binary}.c"} --output ${pkgs.lib.escapeShellArg binary} --cc ${pkgs.stdenv.cc}/bin/cc --cxx ${pkgs.stdenv.cc}/bin/c++ --pkg-config ${pkgs.pkg-config}/bin/pkg-config  # by path: the tool's own wrapper puts its compiler first
    '' else ''
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
