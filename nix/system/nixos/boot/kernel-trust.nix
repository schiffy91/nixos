{ config, lib, pkgs, ... }:
let
  enabled = config.settings.boot.verifyModules || config.settings.boot.lockdown != "none";
  kernel = config.boot.kernelPackages.kernel;
  image = (config.boot.kernelPackages.rebuildImage {
    modulesTree = config.system.modulesTree;
  }).overrideAttrs (old: {
    passthru = (old.passthru or { }) // { inherit (kernel) config; };
    nativeBuildInputs = old.nativeBuildInputs ++ [ pkgs.buildPackages.openssl pkgs.buildPackages.zstd ];
    postBuild = (old.postBuild or "") + ''
      ${pkgs.buildPackages.python3}/bin/python3 ${./kernel-trust/check-modules.py} \
        vmlinux "$moduleCertificates" ${lib.escapeShellArg kernel.modDirVersion} \
        ${lib.escapeShellArgs (map toString config.boot.extraModulePackages)}
    '';
  });
  kernelFile = "${image}/${config.system.boot.loader.kernelFile}";
in {
  # Nixpkgs currently selects the original image independently in the system
  # symlink, bootspec and UKI. Keep the module ABI/build graph unchanged and
  # select the certificate-enriched image at each boot-artifact boundary.
  options.boot.bootspec.writer = lib.mkOption {
    apply = writer: if !enabled then writer else writer + "\n" + ''
      ${pkgs.buildPackages.jq}/bin/jq --arg kernel ${lib.escapeShellArg kernelFile} \
        '."org.nixos.bootspec.v1".kernel = $kernel' \
        "$out/${config.boot.bootspec.filename}" > "$out/boot-trusted.json"
      mv "$out/boot-trusted.json" "$out/${config.boot.bootspec.filename}"
    '';
  };
  config = lib.mkIf enabled {
    system.requiredKernelConfig = with config.lib.kernelConfig; [
      (isYes "MODULE_SIG")
      (isYes "SYSTEM_EXTRA_CERTIFICATE")
    ] ++ lib.optionals (config.settings.boot.lockdown != "none") [
      (isYes "SECURITY_LOCKDOWN_LSM")
      (isYes "SECURITY_LOCKDOWN_LSM_EARLY")
    ];
    assertions = [ {
      assertion = config.settings.boot.lockdown == "none" ||
        !(config.settings.sleep.allowHibernation || config.settings.sleep.hibernateOnSuspend);
      message = "Kernel lockdown prevents hibernation. Choose the sleep policy explicitly before enabling lockdown.";
    } ];
    system.build.kernel = lib.mkForce image;
    system.systemBuilderCommands = lib.mkAfter ''
      ln -sfn ${lib.escapeShellArg kernelFile} "$out/kernel"
    '';
    boot.uki.settings.UKI.Linux = kernelFile;
    boot.kernelParams = [ "module.sig_enforce=1" ] ++
      lib.optional (config.settings.boot.lockdown != "none") "lockdown=${config.settings.boot.lockdown}";
  };
}
