{ pkgs, inputs }:
let
  compiler = inputs.btrc.packages.${pkgs.stdenv.hostPlatform.system}.btrcpy;
  languageServer = inputs.btrc.packages.${pkgs.stdenv.hostPlatform.system}.btrc-lsp;
  source = pkgs.applyPatches {
    name = "btrc-tray-submenus";
    src = inputs.btrc.outPath;
    patches = [ ./submenus.patch ../../../../dev/btrc/native-lsp.patch ];
  };
in {
  # Keep the packaged native reader/sysroot and one shared source for compiler
  # and editor. The patches add tray submenus and typed native LSP analysis.
  btrcpy = pkgs.runCommand "btrcpy-tray" { meta.mainProgram = "btrcpy"; } ''
    mkdir -p "$out/bin"
    cp ${compiler}/bin/btrcpy "$out/bin/btrcpy"
    chmod u+w "$out/bin/btrcpy"
    sed -i 's|^export PYTHONPATH=.*|export PYTHONPATH="${source}"|' "$out/bin/btrcpy"
  '';
  btrc-lsp = pkgs.runCommand "btrc-lsp-tray" { meta.mainProgram = "btrc-lsp"; } ''
    mkdir -p "$out/bin"
    cp ${languageServer}/bin/btrc-lsp "$out/bin/btrc-lsp"
    chmod u+w "$out/bin/btrc-lsp"
    sed -i 's|^export PYTHONPATH=.*|export PYTHONPATH="${source}"|' "$out/bin/btrc-lsp"
    # The editor resolves native bindings with the compiler's reader and target.
    sed -n '/^export BTRC_/p' ${compiler}/bin/btrcpy > native-env.sh
    sed -i '/^export PYTHONPATH=/r native-env.sh' "$out/bin/btrc-lsp"
  '';
}
