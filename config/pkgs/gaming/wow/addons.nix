{ pkgs, overrides ? {} }:  # overrides: settings.apps.gaming.wow.addonOverrides
let
  lib = pkgs.lib;
  lock = builtins.fromJSON (builtins.readFile ./addons.lock.json);
  changes = override: lib.concatStrings (
    map (patch: "patch -p1 < ${patch}\n") (override.patches or [])
    ++ lib.mapAttrsToList (target: source: "cp ${source} ${lib.escapeShellArg target}\n") (override.files or {})
    ++ lib.mapAttrsToList (toc: lines: "printf '\\n%s\\n' ${lib.escapeShellArg (lib.concatStringsSep "\n" lines)} >> ${lib.escapeShellArg toc}\n") (override.tocAppend or {}));
in lib.mapAttrs (name: pin: pkgs.runCommand "wow-addon-${name}" {
  nativeBuildInputs = [ pkgs.unzip pkgs.patch ];
} ''
  mkdir -p "$out"
  cd "$out"
  unzip -q ${pkgs.fetchurl { inherit (pin) url hash; }}
  ${changes (overrides.${name} or {})}
'') lock
