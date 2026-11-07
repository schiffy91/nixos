{ flake ? builtins.getFlake "git+file://${toString ../..}" }:
let
  lib = flake.inputs.nixpkgs.lib;
  names = [
    "FRACTAL-NORTH-Standard-Boot" "FRACTAL-NORTH-Secure-Boot"
    "STEAM-DECK-OLED-Standard-Boot" "STEAM-DECK-OLED-Secure-Boot"
  ];
  defaults = map (name: flake.nixosConfigurations.${name}.config) names;
  selectedKernel = config: "${config.system.build.kernel}/${config.system.boot.loader.kernelFile}";
  imageSelected = config:
    toString config.system.build.kernel != toString config.boot.kernelPackages.kernel &&
    config.boot.uki.settings.UKI.Linux == selectedKernel config;
  prepared = (flake.nixosConfigurations.FRACTAL-NORTH-Secure-Boot.extendModules {
    modules = [ { settings.boot.verifyModules = true; } ];
  }).config;
  locked = (flake.nixosConfigurations.STEAM-DECK-OLED-Secure-Boot.extendModules {
    modules = [ ({ lib, ... }: {
      settings.boot.lockdown = "integrity";
      settings.sleep.allowHibernation = lib.mkForce false;
      settings.sleep.hibernateOnSuspend = lib.mkForce false;
    }) ];
  }).config;
  conflicting = (flake.nixosConfigurations.FRACTAL-NORTH-Secure-Boot.extendModules {
    modules = [ ({ lib, ... }: {
      settings.boot.lockdown = "integrity";
      settings.sleep.allowHibernation = lib.mkForce true;
    }) ];
  }).config;
  recovery = prepared.system.build.recoverySystem.config;
  lockedRecovery = locked.system.build.recoverySystem.config;
in
assert lib.all (config:
  !config.settings.boot.verifyModules && config.settings.boot.lockdown == "none" &&
  toString config.system.build.kernel == toString config.boot.kernelPackages.kernel &&
  lib.all (assertion: assertion.assertion) config.assertions
) defaults;
assert imageSelected prepared && imageSelected recovery && imageSelected locked && imageSelected lockedRecovery;
assert recovery.settings.boot.verifyModules && recovery.settings.boot.lockdown == "none";
assert lockedRecovery.settings.boot.lockdown == "integrity";
assert lib.elem "module.sig_enforce=1" prepared.boot.kernelParams;
assert !(lib.elem "lockdown=integrity" prepared.boot.kernelParams);
assert lib.elem "lockdown=integrity" locked.boot.kernelParams;
assert lib.any (assertion: !assertion.assertion && lib.hasPrefix "Kernel lockdown prevents hibernation." assertion.message) conflicting.assertions;
{
  defaultsUnchanged = true;
  hostAndRecoveryImagesSelected = true;
  moduleVerificationIndependentOfLockdown = true;
  conflictingHibernationRejected = true;
}
