{ pkgs, lib, ... }:
{
  # Disk information
  diskOverrides.device = "/dev/sda"; # This line must exist, but feel free to change the location
  diskOverrides.swapSize = "1G"; # Small swap for a VM
  # VM
  # TODO Update to a newer kernel after Parallels updates their drivers
  # https://github.com/NixOS/nixpkgs/issues/364391
  #boot.kernelPackages = lib.mkForce pkgs.linuxPackages_6_6;
  boot.initrd.availableKernelModules = [ "xhci_pci" "sr_mod" ];
  # ARM64 Packages
  environment.systemPackages = with pkgs; [
    chromium
  ];
  # Qemu
  services.spice-vdagentd.enable = true;
  # Parallels
  #hardware.parallels.enable = lib.mkDefault false;
  # Networking
  networking.useDHCP = lib.mkDefault true;
}