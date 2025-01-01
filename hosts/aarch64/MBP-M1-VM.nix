{ pkgs, config, lib, ... }:
{
  ##### Disk Information #####
  variables.disk.device = "/dev/vda";
  variables.disk.swapSize = "1G"; # Small swap for a VM
  ##### UTM #####
  environment.variables.LIBGL_ALWAYS_SOFTWARE = "1"; # OpenGL drivers are buggy
  services.qemuGuest.enable = true;
  services.spice-vdagentd.enable = true; # For clipboard sharing with Spice
  ##### Boot Configuration #####
  boot.initrd.availableKernelModules = [
    "xhci_pci"
    "virtio_pci"
    "virtio_pci_modern_dev"
    "virtio_gpu"
    "drm"
    "drm_kms_helper"
    "virtio_mmio"
    "virtio_blk"
    "virtio_net"
    "hid_generic"
    "usbhid"
    "9p"
    "9pnet_virtio"
  ];
  boot.initrd.kernelModules = [
    "virtio_balloon"
    "virtio_console"
    "virtio_rng"
    "virtio_gpu"
  ];
  ##### Packages #####
  environment.systemPackages = with pkgs; [
    spice-vdagent
  ];
}