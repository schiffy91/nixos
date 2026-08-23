{ config, inputs, lib, pkgs, pkgs-unstable, ... }:
let
  system = pkgs.stdenv.hostPlatform.system;
  user = config.settings.users.admin.username;
  btrc = inputs.btrc.packages.${system};
  btrcVscode = btrc.btrc-vscode.overrideAttrs (old: {
    npmDepsHash = "sha256-xm6xxb4Nz1kYBJSRBkO3hJmOsw7vZRUkOkZtLQq+MWI=";
    npmDeps = old.npmDeps.overrideAttrs (_: {
      outputHash = "sha256-xm6xxb4Nz1kYBJSRBkO3hJmOsw7vZRUkOkZtLQq+MWI=";
    });
    buildPhase = ''
      runHook preBuild
      bundle_root="$TMPDIR/btrc-vscode"
      python3 - "$PWD/../../.." "$bundle_root" <<'PY'
      import importlib.util
      import sys
      from pathlib import Path

      spec = importlib.util.spec_from_file_location(
          "btrc_vscode_bundle",
          Path("packaging/bundle.py"),
      )
      module = importlib.util.module_from_spec(spec)
      assert spec.loader is not None
      spec.loader.exec_module(module)
      module.ExtensionBundler(
          Path(sys.argv[1]),
          output_root=Path(sys.argv[2]),
      ).bundle()
      PY
      cp -R node_modules "$bundle_root/node_modules"
      cd "$bundle_root"
      npm run typecheck
      npm run compile
      runHook postBuild
    '';
  });
in lib.mkIf (config.settings.apps.enable && config.settings.apps.dev.enable && config.settings.apps.vscode.enable) {
  home-manager.users.${user} = {
    home.packages = [ btrc.btrc-lsp ];
    programs.vscode = {
      enable = true;
      package = pkgs-unstable.vscode;
      mutableExtensionsDir = true;
      profiles.default.extensions = [ btrcVscode ];
    };
  };
}
