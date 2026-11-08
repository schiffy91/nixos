{ lib, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  options = with lib.types; {
    settings.boot.method = mkSetting (enum [ "Disk-Operation" "Standard-Boot" "Secure-Boot"]) "Standard-Boot";
    settings.boot.kernelPackages = mkSetting raw pkgs.linuxPackages_latest;
    settings.boot.touchKeyboard = mkSetting bool false;  # no attached keyboard: every prompt needs an on-screen one
    settings.boot.plymouth.enable = mkSetting bool true;
    settings.boot.plymouth.drmDevice = mkSetting str "";
    settings.boot.pkiBundle = mkSetting str "/var/lib/sbctl";
    settings.boot.autoEnrollKeys = mkSetting bool false; # retained for an actionable error; firmware enrollment is explicit
    settings.boot.previousGenerationLimit = mkSetting int 3;
    settings.boot.timeout = mkSetting int 3;
    settings.tpm.device = mkSetting str "/dev/tpmrm0";
    settings.tpm.versionPath = mkSetting str "/sys/class/tpm/tpm0/tpm_version_major";
  };
}
