{ lib }:
{ pkgs, system, configurations }:
let
  offenders = entry:
    let lanzaboote = entry.configuration.config.boot.lanzaboote;
    in lib.optional (lanzaboote.enable && (lanzaboote.autoEnrollKeys.includeMicrosoftKeys or false))
      "${entry.name}: would enrol Microsoft's Secure Boot keys";
  failures = lib.concatMap offenders configurations;
in {
  name = "own-keys-only";
  value = pkgs.runCommand "own-keys-only-${system}" { } (
    if failures == [ ] then "touch $out"
    else "echo ${lib.escapeShellArg (lib.concatStringsSep "; " failures)} >&2; exit 1");
}
