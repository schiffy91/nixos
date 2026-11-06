{ lib }:
{ pkgs, system, configurations }:
let
  touchModules = [ "hid-multitouch" "i2c-hid-acpi" "i2c-designware-platform" ];
  offenders = entry:
    let
      config = entry.configuration.config;
      initrd = config.boot.initrd;
      touchHost = config.settings.boot.touchKeyboard;
      usesKeyboard = initrd.unl0kr.enable or false;
      recoveryTypable = config.system.build.recoverySystem.config.services.buffyboard.enable or false;
      agentWired = lib.hasAttr "unl0kr-agent" (initrd.systemd.paths or { });
      agent = initrd.systemd.services.unl0kr-agent or { };
      waitsForDrivers = lib.all (unit: lib.elem unit (agent.after or [ ]))
        [ "systemd-modules-load.service" "systemd-udev-trigger.service" ]
        && lib.hasInfix "udevadm settle" (toString (agent.serviceConfig.ExecStartPre or ""));
      retries = (agent.serviceConfig.Restart or "no") == "on-failure"
        && (agent.unitConfig.StartLimitIntervalSec or null) == 0;
      pathText = path:
        if !(lib.isAttrs path) then toString path
        else if path ? outPath then toString path
        else if path ? source then toString path.source
        else "";
      agentPackaged = lib.any (path: lib.hasInfix "unl0kr" (pathText path)) (initrd.systemd.storePaths or [ ]);
      forcedModules = lib.filter (module: !(lib.elem module initrd.kernelModules)) touchModules;
      keyboardShown = ((initrd.unl0kr.settings or { }).keyboard or { }).autohide or true == false;
      drmLoaded = lib.any (module: lib.elem module initrd.kernelModules) [ "amdgpu" "i915" "simpledrm" "virtio_gpu" ];
    in lib.optional (touchHost && !usesKeyboard)
      "${entry.name}: the host has no attached keyboard but the initrd prompt has none either"
    ++ lib.optional (touchHost && !recoveryTypable)
      "${entry.name}: recovery boots to a console with no on-screen keyboard, so it cannot be used without a USB keyboard"
    ++ lib.optionals usesKeyboard (
      lib.optional (!agentWired)
        "${entry.name}: unl0kr is enabled but no unl0kr-agent path unit reaches the initrd, so nothing answers the passphrase request"
      ++ lib.optional (!waitsForDrivers)
        "${entry.name}: unl0kr can start before graphics and touch drivers are loaded"
      ++ lib.optional (!retries)
        "${entry.name}: a transient unl0kr failure can permanently strand the user at a text prompt"
      ++ lib.optional (!agentPackaged)
        "${entry.name}: unl0kr is enabled but its binary is not in the initrd store paths"
      ++ lib.optional (forcedModules != [ ])
        "${entry.name}: the touchscreen cannot be read in initrd, ${lib.concatStringsSep " " forcedModules} are not force-loaded"
      ++ lib.optional (!drmLoaded)
        "${entry.name}: unl0kr draws over DRM but no DRM driver is force-loaded in initrd"
      ++ lib.optional (!keyboardShown)
        "${entry.name}: unl0kr would hide its keyboard; the generated unl0kr.conf replaces the vendor file, so keyboard.autohide must be set false explicitly"
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
