{ config, pkgs, ... }:

{
  config = {
    # System information
    nixpkgs.hostPlatform.system = "aarch64-linux";
    networking.hostName = "MBP-M1-VM";
    shared.driveConfig.swapSize = "17G";
    boot.loader.grub.enable = false;

    # Boot loader
    boot.loader.systemd-boot = {
      enable = true;
      configurationLimit = 3;
      consoleMode = "max";
    };

    # VMWare
    virtualisation.vmware.guest.enable = true;
    # ARM64 Packages
    environment.systemPackages = with pkgs; [
      chromium
      #open-vm-tools
      utm
    ];
  };
}