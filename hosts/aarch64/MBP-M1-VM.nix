{ pkgs, lib, config, ... }:
{
  ##### Disk Information #####
  variables.disk.device = "/dev/nvme0n1";
  variables.disk.swapSize = "1G"; # Small swap for a VM

  # Explicitly disable Parallels support
  #hardware.parallels.enable = false;

  ##### Services #####
  # Enable QEMU guest agent and Spice vdagent service for UTM
  services = {
    qemuGuest.enable = true;
    spice-vdagentd.enable = true; # For clipboard sharing with Spice
    # Other services can be added here
  };

  ##### Boot Configuration #####
  boot = {
    # Include necessary kernel modules for QEMU / KVM virtual machines
    initrd = {
      availableKernelModules = [
        "xhci_pci"
        "nvme"
        "sr_mod"
        "virtio_pci"
        "virtio_blk"
        "virtio_scsi"
        "virtio_net"
        "virtio_gpu"
        "virtiofs"
        "drm"
        "drm_kms_helper"
        "usbhid"
        # Audio modules (uncomment if needed)
        # "snd_hda_intel"
        # "snd_hda_codec"
      ];
    };
    kernelModules = [
      "virtio_balloon"
    ];
    loader = {
      systemd-boot = {
        enable = true;
        configurationLimit = 3;
        consoleMode = "max";
        editor = false;
      };
      efi = {
        canTouchEfiVariables = true;
        efiSysMountPoint = "/boot";
      };
    };
    # Use the latest kernel packages
    kernelPackages = lib.mkForce pkgs.linuxPackages_latest;
  };

  ##### Networking #####
  networking.useDHCP = lib.mkForce true;

  ##### Packages #####
  # ARM64-specific Packages
  environment.systemPackages = with pkgs; [
    chromium
    spice-vdagent
  ];

  ##### Filesystem Configurations #####
  # Configure VirtFS mount
  fileSystems."/mnt/shared_folder" = {
    device = "shared_folder";  # Mount tag from UTM
    fsType = "virtiofs";
    options = [ "rw" "defaults" ];
  };

  # Ensure mount point exists and set permissions
  #system.activationScripts.createVirtFSMountPoint = lib.mkAfter ''
  #  mkdir -p /mnt/shared_folder
  #  chown ${config.variables.user.admin}:users /mnt/shared_folder
  #  chmod 770 /mnt/shared_folder
  #'';
}