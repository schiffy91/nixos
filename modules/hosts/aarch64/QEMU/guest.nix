{ lib, ... }:
{
  boot.initrd.availableKernelModules = [
    "9p"
    "9pnet_virtio"
    "hid_generic"
    "sr_mod"
    "usbhid"
    "virtio_blk"
    "virtio_mmio"
    "virtio_net"
    "virtio_pci"
    "virtio_rng"
    "virtio_scsi"
    "xhci_pci"
  ];

  boot.initrd.kernelModules = [
    "virtio_balloon"
    "virtio_console"
    "virtio_rng"
  ];

  services.qemuGuest.enable = true;
  services.spice-vdagentd.enable = lib.mkDefault false;

  fileSystems."/mnt/shared" = {
    device = "share";
    fsType = "9p";
    options = [
      "trans=virtio"
      "version=9p2000.L"
      "rw"
      "nofail"
      "x-systemd.automount"
    ];
  };
}
