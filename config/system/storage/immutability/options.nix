{ lib, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  options = with lib.types; {
    settings.disk.immutability.enable = mkSetting bool false;
    settings.disk.immutability.mode = mkSetting (enum [ "converge" "reset" "prepare-only" "publish-prepared" "snapshot-only" "restore-a" "restore-b" "restore-c" "restore-generation" "restore-previous" "restore-penultimate" "disabled" ]) "converge";
    settings.disk.immutability.dryRun = mkSetting bool false;
    settings.disk.immutability.enforce.onReboot = mkSetting bool true;
    settings.disk.immutability.nonPersistedGenerations = mkSetting ints.unsigned 3;
    settings.disk.immutability.restoreGeneration = mkSetting ints.positive 1;
    settings.disk.persistence.enable = mkSetting bool true;
    settings.disk.immutability.persist.subvolumeRoot = mkSetting str "@persist";
    settings.disk.immutability.persist.snapshots.cleanName = mkSetting str "CLEAN";
    settings.disk.immutability.persist.neededForBoot = mkSetting (listOf str) [ "/etc/nixos" ];  # make the persisted configuration available before activation
    settings.disk.immutability.persist.paths = lib.mkOption {
      type = listOf str;
      default = [];
      apply = lib.unique;
    };
  };
}
