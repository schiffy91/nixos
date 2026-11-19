{ pkgs, packages }:
let
  # libjpeg-turbo 3.2 exposes tj3Init through a versioned macro. Keep the
  # compiler and language server on the same stdlib native-boundary fix.
  # The LSP also needs the compiler's stdlib provenance on copied byte results.
  btrcpy = packages.btrcpy.overrideAttrs (previous: {
    nativeBuildInputs = (previous.nativeBuildInputs or []) ++ [ pkgs.patch ];
    buildCommand = previous.buildCommand + ''
      library=$("$out/bin/btrcpy" --stdlib-dir)
      runtime="''${library%/src/stdlib}"
      mkdir -p "$out/lib"
      cp -r "$runtime" "$out/lib/btrc"
      chmod -R u+w "$out/lib/btrc"
      patch -d "$out/lib/btrc" -p1 < ${./image-jpeg-init.patch}
      substituteInPlace "$out/bin/btrcpy" --replace-fail "$runtime" "$out/lib/btrc"
    '';
  });
  btrc-lsp = packages.btrc-lsp.overrideAttrs (previous: {
    nativeBuildInputs = (previous.nativeBuildInputs or []) ++ [ pkgs.patch ];
    buildCommand = previous.buildCommand + ''
      runtime=$(sed -n 's|^export PYTHONPATH="\(/nix/store/[^"$]*\).*|\1|p' "$out/bin/btrc-lsp")  # the server runs from sources of its own
      test -f "$runtime/src/devex/lsp/workspace/workspace.py"
      mkdir -p "$out/lib"
      cp -r "$runtime" "$out/lib/btrc"
      chmod -R u+w "$out/lib/btrc"
      patch -d "$out/lib/btrc" -p1 < ${./image-jpeg-init.patch}
      patch -d "$out/lib/btrc" -p1 < ${./lsp-stdlib-origin.patch}
      substituteInPlace "$out/bin/btrc-lsp" --replace-fail "$runtime" "$out/lib/btrc"
    '';
  });
in { inherit btrcpy btrc-lsp; }
