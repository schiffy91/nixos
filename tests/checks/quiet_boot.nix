{ lib }:
{ pkgs, system, configurations }:
let
  requiredParams = [ "quiet" "loglevel=3" "systemd.show_status=auto" "rd.systemd.show_status=auto" ];
  offenders = entry:
    let
      config = entry.configuration.config;
      params = config.boot.kernelParams;
      missing = lib.filter (param: !(lib.elem param params)) requiredParams;
      unlockOwner = config.boot.plymouth.enable || (config.boot.initrd.unl0kr.enable or false);
      encrypted = config.settings.disk.encryption.enable;
    in lib.optional (missing != [ ]) "${entry.name}: boot is not quiet, missing ${lib.concatStringsSep " " missing}"
      ++ lib.optional (encrypted && !unlockOwner)
           "${entry.name}: the disk is encrypted but neither plymouth nor unl0kr owns the initrd screen, so the passphrase prompt lands on a text console";
  failures = lib.concatMap offenders configurations;
in {
  name = "quiet-boot";
  value = pkgs.runCommand "quiet-boot-${system}" { } (
    if failures == [ ] then "touch $out"
    else "echo ${lib.escapeShellArg (lib.concatStringsSep "; " failures)} >&2; exit 1");
}
