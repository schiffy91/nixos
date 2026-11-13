{ lib, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  options = with lib.types; {
    settings.apps.utils.gpuTuning.enable = mkSetting bool false;
    # Applied at boot; the tray's Tuning > GPU menu switches it until the next boot.
    settings.apps.utils.gpuTuning.mode = mkSetting (enum [ "stock" "tuned" ]) "tuned";
    # Flat V/F curve, MSI Afterburner style: every curve point at or above `voltage` (mV)
    # runs at `clock` (MHz) and points below stay stock. Both or neither.
    settings.apps.utils.gpuTuning.voltage = mkSetting (nullOr ints.positive) null;
    settings.apps.utils.gpuTuning.clock = mkSetting (nullOr ints.positive) null;
    # VRAM offset in MSI Afterburner MHz; LACT and NVML count double.
    settings.apps.utils.gpuTuning.memoryOffset = mkSetting int 0;
    # Power limit as a percent of the card's default limit; null keeps the default.
    settings.apps.utils.gpuTuning.powerLimit = mkSetting (nullOr (ints.between 1 150)) null;
  };
}
