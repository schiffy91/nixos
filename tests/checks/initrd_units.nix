{ lib }:
{ pkgs, system, configurations }:
let
  gatedUnits = config: [
    {
      enabled = config.boot.plymouth.enable;
      unit = "plymouth-start.service";
      feature = "plymouth";
    }
  ];
  offenders = entry:
    let units = entry.configuration.config.boot.initrd.systemd.units or { };
    in map (gate: "${entry.name}: ${gate.feature} is disabled but the initrd still defines ${gate.unit}, which systemd refuses for having no ExecStart")
      (lib.filter (gate: !gate.enabled && units ? ${gate.unit}) (gatedUnits entry.configuration.config));
  failures = lib.concatMap offenders configurations;
in {
  name = "initrd-units";
  value = pkgs.runCommand "initrd-units-${system}" { } (
    if failures == [ ] then "touch $out"
    else "echo ${lib.escapeShellArg (lib.concatStringsSep "; " failures)} >&2; exit 1");
}
