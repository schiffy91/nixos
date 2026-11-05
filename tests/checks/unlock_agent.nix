{ lib }:
{ pkgs, system, configurations }:
let
  touchModules = [ "hid-multitouch" "i2c-hid-acpi" "i2c-designware-platform" ];
  offenders = entry:
    let
      config = entry.configuration.config;
      initrd = config.boot.initrd;
      usesKeyboard = initrd.unl0kr.enable or false;
      agentWired = lib.hasAttr "unl0kr-agent" (initrd.systemd.paths or { });
      pathText = path:
        if !(lib.isAttrs path) then toString path
        else if path ? outPath then toString path
        else if path ? source then toString path.source
        else "";
      agentPackaged = lib.any (path: lib.hasInfix "unl0kr" (pathText path)) (initrd.systemd.storePaths or [ ]);
      forcedModules = lib.filter (module: !(lib.elem module initrd.kernelModules)) touchModules;
      drmLoaded = lib.any (module: lib.elem module initrd.kernelModules) [ "amdgpu" "i915" "simpledrm" "virtio_gpu" ];
    in lib.optionals usesKeyboard (
      lib.optional (!agentWired)
        "${entry.name}: unl0kr is enabled but no unl0kr-agent path unit reaches the initrd, so nothing answers the passphrase request"
      ++ lib.optional (!agentPackaged)
        "${entry.name}: unl0kr is enabled but its binary is not in the initrd store paths"
      ++ lib.optional (forcedModules != [ ])
        "${entry.name}: the touchscreen cannot be read in initrd, ${lib.concatStringsSep " " forcedModules} are not force-loaded"
      ++ lib.optional (!drmLoaded)
        "${entry.name}: unl0kr draws over DRM but no DRM driver is force-loaded in initrd"
      ++ lib.optional config.boot.plymouth.enable
        "${entry.name}: plymouth and unl0kr both claim the initrd screen; upstream does not support that"
    );
  failures = lib.concatMap offenders configurations;
in {
  name = "unlock-agent";
  value = pkgs.runCommand "unlock-agent-${system}" { } (
    if failures == [ ] then "touch $out"
    else "echo ${lib.escapeShellArg (lib.concatStringsSep "; " failures)} >&2; exit 1");
}
