{ config, inputs, lib, pkgs, ... }:
{
  imports = [ inputs.logitech-trueforce.nixosModules.default ];

  config = lib.mkIf (config.settings.apps.enable && config.settings.apps.gaming.enable && config.settings.apps.simracing.enable && pkgs.stdenv.hostPlatform.isx86_64) {
    hardware.logitech-trueforce.enable = true;
  };
}
