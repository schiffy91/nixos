{ config, lib, pkgs, ... }: {
  ##### Host Name #####
  networking.hostName = "VM";
  ##### Disk Information #####
  variables.disk.device = "/dev/vda";
  ##### Qemu #####
  services.qemuGuest.enable = true;
  ##### Desktop #####
  variables.desktop.displayServer = lib.mkForce "x11"; # Spice is broken with Wayland. SMH. https://bugzilla.redhat.com/show_bug.cgi?id=2016563
  services.xserver = {
    dpi = builtins.floor (96 * config.variables.displayManager.scalingFactor);
    upscaleDefaultCursor = true;
  };
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
    "virtio_console"
  ];
  ##### Shared Clipboard #####
  services.spice-vdagentd.enable = true;
  ##### Shared Folder #####
  fileSystems."/mnt/shared" = {
    device = "share";
    fsType = "9p";
    options = [ "trans=virtio" "version=9p2000.L" "rw" "nofail" ];
  };
  ##### Security #####
  security.sudo.wheelNeedsPassword = false;
  ##### Packages #####
  environment.systemPackages = with pkgs; [
    spice-vdagent
    spice-protocol
  ];
}