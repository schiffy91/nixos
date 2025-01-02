{ pkgs, ... }:
{
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
  ];
  boot.kernelParams = [
    "console=tty0"
  ];
  ##### Packages #####
  environment.systemPackages = with pkgs; [ ];
}
