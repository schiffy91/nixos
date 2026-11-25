{ lib, pkgs, ... }:
{
  networking.hostName = "VM-DEVELOPER";

  settings = {
    apps = {
      gaming.enable = false;
      # Google Chrome, Cider, Mullvad GUI, and 1Password GUI target x86_64-linux.
      # Keep the full ARM64 dev and utility stack, plus Chromium and 1Password CLI.
      utils = {
        desktop.enable = false;
        onePassword.enable = false;
        packages = with pkgs; [
          _1password-cli
          chromium
          ethtool
          mesa-demos
          pciutils
          tcpdump
          usbutils
          vulkan-tools
        ];
      };
      dev.packages = with pkgs; [
        btop
        clang
        cmake
        fd
        gcc
        gdb
        gnumake
        htop
        jq
        meson
        ninja
        pkg-config
        python3
        ripgrep
        tmux
      ];
    };
    boot.timeout = 0;
    nix = {
      minFreeGiB = 4;
      maxFreeGiB = 12;
    };
    disk = {
      device = "/dev/vda";
      encryption.enable = false;
      immutability.enable = false;
      persistence.enable = false;
      swap.size = "4G";
    };
    networking = {
      identityAgent = "none";
      lanSubnet = "10.0.2.0/24";
    };
    users.admin = {
      autoLogin.enable = true;
      autoLock.enable = false;
      autoUnlockWallet.enabled = false;
    };
  };

  security.sudo.wheelNeedsPassword = lib.mkForce false;

  boot.initrd.availableKernelModules = [
    "9p"
    "9pnet_virtio"
    "hid_generic"
    "sr_mod"
    "usbhid"
    "virtio_blk"
    "virtio_gpu"
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
    "virtio_gpu"
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
      "console=ttyAMA0,115200n8"
      "console=tty0"
    ];
  };
}
