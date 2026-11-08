{ lib }:
{ pkgs, system, configurations }:
let
  requiredParams = [ "quiet" "systemd.show_status=auto" "rd.systemd.show_status=auto" ];
  offenders = entry:
    let
      config = entry.configuration.config;
      params = config.boot.kernelParams;
      missing = lib.filter (param: !(lib.elem param params)) requiredParams;
      loglevels = lib.filter (lib.hasPrefix "loglevel=") params;
      unlockOwner = config.boot.plymouth.enable || (config.boot.initrd.unl0kr.enable or false);
      encrypted = config.settings.disk.encryption.enable;
    in lib.optional (missing != [ ]) "${entry.name}: boot is not quiet, missing ${lib.concatStringsSep " " missing}"
      ++ lib.optional (loglevels != [ "loglevel=${toString config.boot.consoleLogLevel}" ] || config.boot.consoleLogLevel > 3)
           "${entry.name}: conflicting or verbose kernel console log levels"
      ++ lib.optional (encrypted && !unlockOwner)
           "${entry.name}: the disk is encrypted but neither plymouth nor unl0kr owns the initrd screen, so the passphrase prompt lands on a text console";
  failures = lib.concatMap offenders configurations;
in {
  name = "quiet-boot";
  value = pkgs.runCommand "quiet-boot-${system}" { } (
    if failures == [ ] then "touch $out"
    else "echo ${lib.escapeShellArg (lib.concatStringsSep "; " failures)} >&2; exit 1");
}
