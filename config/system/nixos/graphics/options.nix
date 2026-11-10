{ lib, config, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
  mkDerived = type: lib.mkOption { inherit type; readOnly = true; };
in {
  options = with lib.types; {
    settings.graphics.gpus = mkSetting (attrsOf (submodule {
      options = {
        enable = mkSetting bool true;  # false keeps a card that firmware has off
        driver = mkSetting (enum [ "amdgpu" "nvidia" ]) null;
        pci = mkSetting str null;  # as lspci -D prints it: 0000:01:00.0
        audioPci = mkSetting (nullOr str) null;
        connectors = mkSetting (attrsOf int) {};  # physical ports per type: { DP = 3; }
      };
    })) {};
    settings.graphics.display = mkSetting str "";  # the GPU the monitors hang off
    settings.graphics.render = mkSetting str config.settings.graphics.display;  # the GPU that draws
    settings.graphics.environment = mkDerived (attrsOf str);  # what a program needs to draw on the render GPU
    settings.graphics.connectors = mkDerived (attrsOf (attrsOf (listOf str)));  # kernel names: connectors.nvidia.DP
  };
}
