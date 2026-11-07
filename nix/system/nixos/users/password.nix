{ config, inputs, lib, pkgs, ... }:
let
  admin = config.settings.users.admin;
  helper = import ../../../apps/utils/nixosctl/package.nix {
    inherit pkgs lib;
    btrcpy = inputs.btrc.packages.${pkgs.stdenv.hostPlatform.system}.btrcpy;
  };
in {
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
