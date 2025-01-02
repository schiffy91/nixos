{ pkgs, ... }:
let kernelPackage = "linux_6_11"; in # Parallels Tools is broken on anything greater than 6.11. SMH
{
  ##### Host Name #####
  networking.hostName = "VM";
  ##### Disk Information #####
  variables.disk.device = "/dev/sda";
  variables.disk.swapSize = "1G"; # Small swap for a VM
  ##### Parallels #####
  hardware.parallels = {
    enable = true;
    package = pkgs.linuxKernel.packages.${kernelPackage}.prl-tools;
  };
  boot.kernelPackages = pkgs.linuxPackages_6_11;
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
    "prl_fs_freeze"
    "prl_fs"
    "prl_tg"
  ];
  boot.initrd.kernelModules = [
    "virtio_gpu"
    "virtio_balloon"
    "virtio_rng"
  ];
  ##### Packages #####
  environment.systemPackages = with pkgs; [
      linuxKernel.packages.${kernelPackage}.prl-tools
  ];
}