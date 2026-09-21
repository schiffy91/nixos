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
      python3 packaging/bundle.py --repository-root "$PWD/../../.." --output-root "$bundle_root"
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
