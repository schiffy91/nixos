{ config, inputs, lib, pkgs, utils, ... }:
let
  admin = config.settings.users.admin;
  containsPassword = path: path == "/" || path == admin.hashedPasswordFile
    || lib.hasPrefix "${lib.removeSuffix "/" path}/" admin.hashedPasswordFile;
  passwordVolumes = lib.sort (a: b: builtins.stringLength a.mountPoint > builtins.stringLength b.mountPoint)
    (lib.filter (volume: containsPassword volume.mountPoint) config.settings.disk.subvolumes.volumes);
  resetPassword = passwordVolumes != [] && (builtins.head passwordVolumes).resetOnBoot;
  helper = import ../../../apps/utils/nixosctl/package.nix {
    inherit pkgs lib;
    btrcpy = inputs.btrc.packages.${pkgs.stdenv.hostPlatform.system}.btrcpy;
  };
in {
  assertions = [
    {
      assertion = !(builtins.hasAttr admin.hashedPasswordFile config.fileSystems);
      message = "settings.users.admin.hashedPasswordFile must not itself be a filesystem mountpoint: password updates require atomic file replacement. Mount its parent directory instead.";
    }
    {
      assertion = lib.all utils.fsNeededForBoot
        (lib.filter (fs: containsPassword fs.mountPoint) (builtins.attrValues config.fileSystems));
      message = "Every filesystem containing settings.users.admin.hashedPasswordFile must be mounted before activation (neededForBoot).";
    }
    {
      assertion = !config.settings.disk.immutability.enable || !resetPassword
        || (config.settings.disk.persistence.enable
          && lib.any containsPassword config.settings.disk.immutability.persist.neededForBoot);
      message = "settings.users.admin.hashedPasswordFile must survive immutable-root resets and be available before activation.";
    }
  ];
  users.users.${admin.username}.hashedPasswordFile = admin.hashedPasswordFile;
  system.activationScripts.passwordFile = {
    deps = [ "specialfs" ];
    text = ''
      # Preserve the currently effective password on upgrade. Never import an
      # authoritative credential from the editable repository.
      ${helper}/bin/nixosctl-password-file ${lib.escapeShellArg admin.hashedPasswordFile} ${lib.escapeShellArg admin.username} || exit 1
    '';
  };
  system.activationScripts.users.deps = [ "passwordFile" ];
}
