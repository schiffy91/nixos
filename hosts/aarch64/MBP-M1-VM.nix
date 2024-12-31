{ pkgs, lib, config, ... }:
{
  # Disk information
  diskOverrides.device = "/dev/nvme0n1"; # This line must exist, but feel free to change the location
  diskOverrides.swapSize = "1G"; # Small swap for a VM
  # VM
  boot.initrd.availableKernelModules = [ "virtio_pci" "xhci_pci" "usb_storage" "usbhid" ];
  # ARM64 Packages
  environment.systemPackages = with pkgs; [
    chromium
  ];
  # Qemu
  services.spice-vdagentd.enable = true;
  services.spice-webdavd.enable = true;
  # Parallels
  hardware.parallels.enable = lib.mkDefault false;
  # TODO Update to a newer kernel after Parallels updates their drivers
  # https://github.com/NixOS/nixpkgs/issues/364391
  boot.kernelPackages = lib.mkIf config.hardware.parallels.enable (lib.mkForce pkgs.linuxPackages_6_6);
  # Networking
  networking.useDHCP = lib.mkDefault true;
}