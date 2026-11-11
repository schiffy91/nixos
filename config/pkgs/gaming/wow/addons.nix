{ pkgs }:
let
  lock = builtins.fromJSON (builtins.readFile ./addons.lock.json);
in pkgs.lib.mapAttrs (name: pin: pkgs.runCommand "wow-addon-${name}" {
  nativeBuildInputs = [ pkgs.unzip ] ++ pkgs.lib.optional (name == "foreverFadeChat") pkgs.patch;
} ''
  mkdir -p "$out"
  cd "$out"
  unzip -q ${pkgs.fetchurl { inherit (pin) url hash; }}
  ${pkgs.lib.optionalString (name == "foreverFadeChat") ''
    patch -p1 < ${./forever-fade-chat.patch}
  ''}
'') lock
