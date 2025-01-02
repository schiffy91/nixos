{ pkgs, config, lib, ... }:
{
  ##### Disk Information #####
  variables.disk.device = "/dev/vda";
  variables.disk.swapSize = "1G"; # Small swap for a VM
  ##### UTM #####
  services.qemuGuest.enable = true;
  services.spice-vdagentd.enable = true; # For clipboard sharing with Spice
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
  boot.kernelParams = [
    "console=tty1"
    "video=virtio-ramfb:1920x1080"
  ];
  ##### Packages #####
  environment.systemPackages = with pkgs; [
    spice-vdagent
  ];
}