{ pkgs, config, lib, ... }:
{
  ##### Host Name #####
  networking.hostName = "VM";
  ##### Disk Information #####
  variables.disk.device = "/dev/vda";
  variables.disk.swapSize = "1G"; # Small swap for a VM
  ##### Boot Configuration #####
  boot.initrd.availableKernelModules = [
    "virtio_pci"
    "virtio_blk"
    "virtio_net"
    "virtio_gpu"
    "drm"
    "drm_kms_helper"
    "virtio_snd"
    "virtiofs"
    "virtio_rng"
    "virtio_balloon"
    "virtio_console"
    "xhci_pci"
    "hid_generic"
    "usbhid"
    "fuse"
  ];
  ##### Shared Clipboard #####
  services.spice-vdagentd.enable = true; # For clipboard sharing with Spice #TODO Fix this
  ##### Shared Folder #####
  fileSystems."/mnt/shared" = {
    device = "share";  # Mount tag from UTM
    fsType = "virtiofs";
    options = [ "rw" "nofail" ];
  };
  users.users.${config.variables.user.admin}.extraGroups = [ "fuse" ];
  ##### Packages #####
  environment.systemPackages = with pkgs; [ 
    spice-vdagent
    fuse
  ];
}
