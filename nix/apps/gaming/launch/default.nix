{ config, pkgs, lib, ... }:
let
  package = import ./package.nix {
    inherit pkgs;
    inherit (config.settings.apps.gaming) cpuAffinity;
  };
in {
  _module.args.gameLaunch = {
    inherit package;
    prefix = "${package}/bin/game-run env";
  };
  users.users.${config.settings.users.admin.username}.packages =
    lib.optionals (config.settings.apps.enable && config.settings.apps.gaming.enable) [ package ];
}
