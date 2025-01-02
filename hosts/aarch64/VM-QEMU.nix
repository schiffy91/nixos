{ pkgs, ... }:
{
  ##### Host Name #####
  networking.hostName = "VM";
  ##### Disk Information #####
  variables.disk.device = "/dev/vda";
  variables.disk.swapSize = "1G";
  ##### Qemu #####
  services.qemuGuest.enable = true;
  ##### Boot Configuration #####
  boot.initrd.availableKernelModules = [
    "xhci_pci"
    "virtio_pci"
    "virtio_pci_modern_dev"
    "drm"
    "drm_kms_helper"
    "virtio_mmio"
    "virtio_blk"
    "virtio_net"
    "hid_generic"
    "usbhid"
    "9p"
    "9pnet_virtio"
    "snd_hda_codec"
    "snd_hda_core"
  ];
  boot.initrd.kernelModules = [
    "virtio_gpu"
    "virtio_balloon"
    "virtio_rng"
  ];
  ##### Shared Clipboard #####
  services.spice-vdagentd.enable = true;
  ##### Shared Folder #####
  fileSystems."/mnt/shared" = {
    device = "share";  # Mount tag from UTM
    fsType = "virtiofs";
    options = [ "rw" "nofail" ];
  };
  ##### Packages #####
  environment.systemPackages = with pkgs; [
    spice-vdagent
  ];
}