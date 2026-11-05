{ lib, mkInstaller, bootableTargets, hostOutputName, prebuiltSources }:
{ pkgs, system, prebuiltHost, touchHost ? null }:
let
  generic = (mkInstaller system [ ]).config;
  prebuilt = (mkInstaller system [ prebuiltHost ]).config;
  repositoryScript = generic.systemd.services.installer-repository.script or "";
  manifestOf = cfg: builtins.fromJSON (builtins.unsafeDiscardStringContext
    cfg.environment.etc."nixos-installer/prebuilt.json".text);
  manifest = manifestOf prebuilt;
  expectedKeys = map (target: "${hostOutputName prebuiltHost}-${target}") bootableTargets;
  readFields = [ "system" "disko" "mount" ];
  failures = lib.optional (generic.users.users.root.openssh.authorizedKeys.keys == [ ]) "root has no authorized keys"
    ++ lib.optional (!(generic.systemd.services ? installer-repository)) "no service places /etc/nixos"
    ++ lib.optional (!(lib.hasInfix "--chmod=u+w" repositoryScript)) "/etc/nixos would not be writable"
    ++ lib.optional (!(lib.any (p: (p.name or "") == "nixosctl") generic.environment.systemPackages)) "nixosctl is not installed"
    ++ lib.optional (builtins.attrNames manifest != lib.sort (a: b: a < b) expectedKeys)
         "prebuilt manifest does not key every boot target by the name the flake exports"
    ++ lib.optional (!(lib.all (key: lib.all (field:
         lib.hasPrefix "/nix/store/" (manifest.${key}.${field} or "")) readFields) expectedKeys))
         "prebuilt manifest does not name a store path for system, disko and mount"
    ++ lib.optional (lib.length prebuilt.isoImage.storeContents
         <= lib.length generic.isoImage.storeContents + lib.length bootableTargets)
         "prebuilt ISO carries too little to install offline"
    ++ lib.optional (prebuiltSources == [ ]) "prebuilt ISO carries no flake input sources"
    ++ lib.optional (manifestOf generic != { }) "the generic ISO must advertise no prebuilt system"
    ++ lib.optional (generic.services.buffyboard.enable)
         "the generic ISO draws an on-screen keyboard over a console that does not need one"
    ++ lib.optional (touchHost != null && !(mkInstaller system [ touchHost ]).config.services.buffyboard.enable)
         "${if touchHost == null then "" else hostOutputName touchHost} unlocks by touch but its installer has no on-screen keyboard, so installing needs a USB keyboard";
in {
  name = "installer-contract";
  value = pkgs.runCommand "installer-contract-${system}" { } (
    if failures == [ ] then "touch $out"
    else "echo ${lib.escapeShellArg (lib.concatStringsSep "; " failures)} >&2; exit 1");
}
