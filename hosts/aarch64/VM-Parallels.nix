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
  boot.kernelModules = [
    "prl_tg"  # Parallels Tools communications
    "prl_fs"  # Shared folders
    "prl_fs_freeze"  # File system synchronization
    "prl_eth" # Network driver
    "prl_vid" # Video driver
    "prl_clipboard" # Clipboard sharing
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