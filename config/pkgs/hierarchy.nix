{ config, options, lib, pkgs, ... }:
let
  apps = config.settings.apps;
  hierarchy = import ../lib/enable-hierarchy.nix { inherit lib; };
  requires = child: parent: {
    assertion = !(lib.getAttrFromPath (child ++ [ "enable" ]) apps)
      || lib.getAttrFromPath (parent ++ [ "enable" ]) apps;
    message = "settings.apps.${lib.concatStringsSep "." child}.enable requires settings.apps.${lib.concatStringsSep "." parent}.enable.";
  };
in {
  assertions = hierarchy [ "settings" "apps" ] options.settings.apps apps null
    ++ map (app: requires [ "gaming" app ] [ "gaming" "steam" "protonCustom" ])
      [ "assettocorsa" "battlenet" "rocksmith" ]
    ++ [ (requires [ "gaming" "wow" ] [ "gaming" "battlenet" ]) ]
    ++ map (app: {
      assertion = !apps.gaming.${app}.enable || (pkgs.stdenv.hostPlatform.isLinux && pkgs.stdenv.hostPlatform.isx86_64);
      message = "settings.apps.gaming.${app}.enable requires an x86_64 Linux host.";
    }) [ "steam" "assettocorsa" "battlenet" "rocksmith" "semu" "simracing" "wow" ];
}
