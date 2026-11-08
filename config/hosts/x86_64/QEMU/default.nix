{ lib, ... }:
{
  networking.hostName = "QEMU";

  settings = {
    apps.enable = false;
    boot.timeout = 0;
    desktop.enable = false;
    disk = {
      device = "/dev/vda";
      encryption.enable = false;
      immutability = {
        enable = true;
        mode = "reset";
      };
      swap.size = "4G";
    };
    networking = {
      identityAgent = "none";
      lanSubnet = "10.0.2.0/24";
    };
    users.admin = {
      autoLogin.enable = false;
      autoLock.enable = false;
      autoUnlockWallet.enabled = false;
      homeManager.enable = false;
    };
    users.agent.enable = false;
  };

  security.sudo.wheelNeedsPassword = lib.mkForce false;

boot.initrd.availableKernelModules = [
    "9p"
    "9pnet_virtio"
    "ata_piix"
    "hid_generic"
    "sr_mod"
    "usbhid"
    "virtio_blk"
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

boot = {
    loader = {
      efi.canTouchEfiVariables = lib.mkDefault true;
      timeout = lib.mkDefault 0;
    };
    kernelParams = [
      "console=ttyS0,115200n8"
      "console=tty0"
    ];
  };
}
