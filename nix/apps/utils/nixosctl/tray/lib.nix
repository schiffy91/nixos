{ pkgs, inputs }:
let
  compiler = inputs.btrc.packages.${pkgs.stdenv.hostPlatform.system}.btrcpy;
  source = pkgs.applyPatches {
    name = "btrc-tray-submenus";
    src = inputs.btrc.outPath;
    patches = [ ./submenus.patch ];
  };
in {
  # Keep the packaged native-header reader/sysroot while extending its Linux
  # tray provider. Remove this patch once the pinned stdlib supports submenus.
  btrcpy = pkgs.runCommand "btrcpy-tray" { meta.mainProgram = "btrcpy"; } ''
    mkdir -p "$out/bin"
    cp ${compiler}/bin/btrcpy "$out/bin/btrcpy"
    chmod u+w "$out/bin/btrcpy"
    sed -i 's|^export PYTHONPATH=.*|export PYTHONPATH="${source}"|' "$out/bin/btrcpy"
  '';
}
