{ inputs, config, lib, ... }: 
lib.mkMerge [
  lib.mkIf config.settings.immutability.enabled {
    boot.readOnlyNixStore = true;
  }
]
