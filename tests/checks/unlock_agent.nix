{ lib }:
{ pkgs, system, configurations }:
let
  touchModules = [ "hid-multitouch" "i2c-hid-acpi" "i2c-designware-platform" ];
  offenders = entry:
    let
      config = entry.configuration.config;
      initrd = config.boot.initrd;
      touchHost = config.settings.boot.touchKeyboard;
      usesKeyboard = config.boot.plymouth.enable && (config.boot.plymouth.package.touchKeyboard or false);
      recovery = config.system.build.recoverySystem.config;
      recoveryTypable = (recovery.services.buffyboard.enable or false)
        && lib.elem "uinput" recovery.boot.kernelModules
        && lib.elem "dev-fb0.device" recovery.systemd.services.buffyboard.after;
      agentWired = lib.elem config.boot.plymouth.package initrd.systemd.packages;
      waitsForDrivers = lib.elem "systemd-modules-load.service"
        (initrd.systemd.services.plymouth-start.after or [ ]);
      missingModules = lib.filter (module:
        !(lib.elem module (initrd.kernelModules ++ initrd.availableKernelModules))) touchModules;
      drmLoaded = lib.any (module: lib.elem module initrd.kernelModules) [ "amdgpu" "i915" "simpledrm" "virtio_gpu" ];
    in lib.optionals touchHost (
      lib.optional (!usesKeyboard)
        "${entry.name}: the handheld needs Plymouth's touch keyboard in initrd"
      ++ lib.optional (!recoveryTypable)
        "${entry.name}: recovery boots to a console with no on-screen keyboard"
      ++ lib.optional (!agentWired)
        "${entry.name}: no Plymouth path unit answers initrd password requests"
      ++ lib.optional (!waitsForDrivers)
        "${entry.name}: Plymouth can start before the graphics driver loads"
      ++ lib.optional (missingModules != [ ])
        "${entry.name}: touchscreen drivers are missing from initrd: ${lib.concatStringsSep " " missingModules}"
      ++ lib.optional (!drmLoaded)
        "${entry.name}: no graphics driver is force-loaded for the touch keyboard"
      ++ lib.optional (initrd.unl0kr.enable or false)
        "${entry.name}: unl0kr would compete with Plymouth for the initrd screen"
    );
  failures = lib.concatMap offenders configurations;
in {
  name = "unlock-agent";
  value = pkgs.runCommand "unlock-agent-${system}" { } (
    if failures == [ ] then "touch $out"
    else "echo ${lib.escapeShellArg (lib.concatStringsSep "; " failures)} >&2; exit 1");
}
