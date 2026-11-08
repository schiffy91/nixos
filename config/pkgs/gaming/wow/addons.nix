{ pkgs }:
let
  lock = builtins.fromJSON (builtins.readFile ./addons.lock.json);
in pkgs.lib.mapAttrs (name: pin: pkgs.runCommand "wow-addon-${name}" {
  nativeBuildInputs = [ pkgs.unzip ];
} ''
  mkdir -p "$out"
  cd "$out"
  unzip -q ${pkgs.fetchurl { inherit (pin) url hash; }}
'') lock
