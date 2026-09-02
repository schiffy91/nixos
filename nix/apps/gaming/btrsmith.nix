{ config, inputs, lib, pkgs, ... }:
let
  enabled = config.settings.apps.enable && config.settings.apps.gaming.enable && config.settings.apps.btrsmith.enable;
  system = pkgs.stdenv.hostPlatform.system;
  home = "/home/${config.settings.users.admin.username}";
  inputAvailable = inputs ? btrsmith && inputs.btrsmith ? packages && builtins.hasAttr system inputs.btrsmith.packages && inputs.btrsmith.packages.${system} ? btrsmith;
in lib.mkIf enabled {
  assertions = [{
    assertion = inputAvailable;
    message = "settings.apps.btrsmith.enable requires an exact BTRSmith flake input with packages.${system}.btrsmith";
  }];
  environment.systemPackages = lib.optionals inputAvailable [ inputs.btrsmith.packages.${system}.btrsmith ];
  settings.disk.immutability.persist.paths = [ "${home}/.local/state/com.schiffy91.btrsmith" ];
}
