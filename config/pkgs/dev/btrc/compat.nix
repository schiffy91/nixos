{ pkgs, packages }:
{
  inherit (packages) btrcpy;
  btrc-lsp = packages.btrc-lsp.overrideAttrs (previous: {
    nativeBuildInputs = (previous.nativeBuildInputs or []) ++ [ pkgs.patch ];
    buildCommand = previous.buildCommand + ''
      runtime=$(sed -n 's|^export PYTHONPATH="\(/nix/store/[^"$]*\).*|\1|p' "$out/bin/btrc-lsp")  # the server runs from sources of its own
      test -f "$runtime/src/devex/lsp/workspace/workspace.py"
      mkdir -p "$out/lib"
      cp -r "$runtime" "$out/lib/btrc"
      chmod -R u+w "$out/lib/btrc"
      patch -d "$out/lib/btrc" -p1 < ${./lsp-stdlib-origin.patch}
      substituteInPlace "$out/bin/btrc-lsp" --replace-fail "$runtime" "$out/lib/btrc"
    '';
  });
}
