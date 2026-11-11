{ pkgs }:
let
  lock = builtins.fromJSON (builtins.readFile ./addons.lock.json);
in pkgs.lib.mapAttrs (name: pin: pkgs.runCommand "wow-addon-${name}" {
  nativeBuildInputs = [ pkgs.unzip pkgs.patch ];
} ''
  mkdir -p "$out"
  cd "$out"
  unzip -q ${pkgs.fetchurl { inherit (pin) url hash; }}
  ${pkgs.lib.optionalString (name == "foreverFadeChat") ''
    patch -p1 < ${./forever-fade-chat.patch}
  ''}
  ${pkgs.lib.optionalString (name == "flareUI") ''
    patch -p1 < ${./flareui-layout.patch}
    cp ${./display-layout.lua} FlareUI/DisplayLayout.lua
    printf '\nDisplayLayout.lua\n' >> FlareUI/FlareUI.toc
  ''}
'') lock
