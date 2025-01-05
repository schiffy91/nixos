#TODO 100% CPU Utilization
{ pkgs, config, ... }:
{
  ##### Host Name #####
  networking.hostName = "VM";
  ##### Disk Information #####
  variables.disk.device = "/dev/vda";
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
  services.spice-vdagentd.enable = true;
  ##### Shared Folder #####
  fileSystems."/mnt/shared" = {
    device = "share";  # Mount tag from UTM
    fsType = "virtiofs";
    options = [ "rw" "nofail" ];
  };
  users.users.${config.variables.user.admin.username}.extraGroups = [ "fuse" ];
  ##### Security #####
  security.sudo.wheelNeedsPassword = false;
  ##### Packages #####
  environment.systemPackages = with pkgs; [ 
    spice-vdagent
    fuse
  ];
}