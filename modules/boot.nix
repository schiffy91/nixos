{ pkgs, lib, inputs, ... }:
let
  sbctl = "${pkgs.sbctl}/bin/sbctl";
in
{
  imports = [ inputs.lanzaboote.nixosModules.lanzaboote ];
  boot = {
    loader = {
      timeout = 5;
      systemd-boot.enable = lib.mkForce false;  # Disable systemd-boot when lanzaboote active
      efi = {
        canTouchEfiVariables = true;
        efiSysMountPoint = "/boot";
      };
    };
    lanzaboote = {
      enable = true;
      pkiBundle = "/etc/secureboot";
    };
    initrd.systemd.enable = true;
    plymouth = {
      enable = true;
      theme = "breeze";
    };
    kernelParams = [ "quiet" ];
    kernelPackages = pkgs.linuxPackages_latest;
  };

  # Move service to run after system is fully booted
  systemd.services.enroll-secure-boot-keys = {
    description = "Enroll Secure Boot keys";
    wantedBy = [ "multi-user.target" ];
    after = [ "multi-user.target" ];
    unitConfig.ConditionPathExists = "!/etc/secureboot-enrolled";
    
    serviceConfig = {
      Type = "oneshot";
      RemainAfterExit = true;
    };

    script = ''
      set -euo pipefail
      # Check if secure boot is possible
      if ! ${sbctl} status | grep -q "Setup Mode:.*enabled"; then
        logger -t enroll-secure-boot-keys "Secure Boot is not enabled or not supported. Skipping enrollment."
        exit 0
      fi

      logger -t enroll-secure-boot-keys "Enrolling Secure Boot keys..."
      if ! ${sbctl} enroll-keys --microsoft --force; then
        logger -t enroll-secure-boot-keys "Error: Failed to enroll Secure Boot keys."
        journalctl -u enroll-secure-boot-keys.service -b -n 50 --no-pager
        exit 1
      fi

      ${sbctl} verify

      if ! ${sbctl} verify; then
        logger -t enroll-secure-boot-keys "Error: Failed to verify Secure Boot keys."
        journalctl -u enroll-secure-boot-keys.service -b -n 50 --no-pager
        exit 1
      fi

      touch /etc/secureboot-enrolled
      logger -t enroll-secure-boot-keys "Secure Boot keys enrolled and verified successfully."
    '';
  };
}