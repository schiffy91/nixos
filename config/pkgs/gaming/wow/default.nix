{ config, pkgs, lib, inputs, ... }:
let
  cfg = config.settings.apps.wow;
  enabled = config.settings.apps.enable
    && config.settings.apps.gaming.enable
    && config.settings.apps.battlenet.enable
    && cfg.enable
    && pkgs.stdenv.hostPlatform.isx86_64;
  user = config.settings.users.admin.username;
  group = config.users.users.${user}.group;
  forever = "/home/${user}/Games/Battle.net/prefix/drive_c/Program Files (x86)/World of Warcraft/_forever_";
  lock = builtins.fromJSON (builtins.readFile ./addons.lock.json);  # wow-addons-lock refreshes this
  # The lock stores the zip's own sha256, so refreshing it needs nothing but a download.
  fetch = name: pin: pkgs.runCommand "wow-addon-${name}" {
    nativeBuildInputs = [ pkgs.unzip ];
  } ''
    mkdir -p "$out" && cd "$out"
    unzip -q ${pkgs.fetchurl { inherit (pin) url hash; }}  # a release zip holds one or more addon folders
  '';
  addons = pkgs.symlinkJoin {
    name = "wow-forever-addons";
    paths = lib.mapAttrsToList fetch lock;
  };
  # Refreshes GitHub releases and hashes explicit download URLs. nixosctl upgrade runs it.
  lockTool = import ./package.nix { inherit pkgs inputs; };

in lib.mkIf enabled {
  environment.systemPackages = [ lockTool ];
  environment.etc."${config.settings.apps.nixosctl.upgradeHooksDir}/wow-addons".source = "${lockTool}/bin/wow-addons-lock";
  system.activationScripts.wowAddons = lib.stringAfter [ "users" ] ''
    if [ -d "${forever}" ]; then
      ${pkgs.coreutils}/bin/install -d -o ${user} -g ${group} "${forever}/Interface"
      target="${forever}/Interface/AddOns"
      if [ ! -L "$target" ]; then ${pkgs.coreutils}/bin/rm -rf "$target"; fi  # drop any hand-installed tree
      ${pkgs.coreutils}/bin/ln -sfn ${addons} "$target"
      ${pkgs.coreutils}/bin/chown -h ${user}:${group} "$target"
      # Seed a shared profile once; WoW owns subsequent in-game edits.
      for account in "${forever}/WTF/Account/"*; do
        [ -d "$account" ] || continue
        profile="$account/SavedVariables/Details.lua"
        if [ ! -e "$profile" ]; then
          ${pkgs.coreutils}/bin/install -D -m 0644 -o ${user} -g ${group} ${./details-profile.lua} "$profile"
        fi
      done
    fi
  '';
}
