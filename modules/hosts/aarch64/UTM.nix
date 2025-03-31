{ pkgs, lib, ... }: {
  ##### Host Name #####
  networking.hostName = "VM";
  ##### Disk Information #####
  settings.disk.device = "/dev/vda";
  settings.disk.encryption.enable = false;
  settings.disk.swap.enable = false;
  settings.disk.immutability.enable = true;
  ##### Qemu #####
  services.qemuGuest.enable = true;
  ##### Desktop #####
  settings.desktop.environment = lib.mkForce "gnome-wayland"; # Spice is broken with Wayland. SMH. https://bugzilla.redhat.com/show_bug.cgi?id=2016563
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
    "virtio_net"
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
  settings.user.admin.autoLogin.enable = true;
  settings.user.admin.autoLock.enable = false;
  ##### Packages #####
  environment.systemPackages = with pkgs; [
    spice-vdagent
    spice-protocol
  ];
  ##### Networking #####
  settings.networking.lanSubnet = "192.168.64.0/24";
}