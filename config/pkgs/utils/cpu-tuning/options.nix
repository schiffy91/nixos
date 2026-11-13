{ lib, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  options = with lib.types; {
    settings.apps.utils.cpuTuning.enable = mkSetting bool false;
    # Applied at boot; the tray's Tuning > CPU menu switches it until the next boot.
    # "bios" keeps (and records) the counts the firmware set; "software" writes `offsets`.
    settings.apps.utils.cpuTuning.mode = mkSetting (enum [ "stock" "bios" "software" ]) "software";
    # AMD Curve Optimizer counts per physical core, index = Linux core id
    # (CCD = core / 8, slot = core % 8). Runtime only: the SMU forgets them at reboot.
    settings.apps.utils.cpuTuning.offsets = mkSetting (listOf (ints.between (-50) 30)) [];
  };
}
