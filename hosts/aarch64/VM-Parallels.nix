#TODO Clipboard doesn't work
{ pkgs, ... }:
let
  kernel_version = "6_6"; # Parallels Tools is broken on anything greater than 6.11. SMH
  kernelPackage = "linux_${kernel_version}";
  linuxPackage = "linuxPackages_${kernel_version}";
in
{
  ##### Host Name #####
  networking.hostName = "VM";
  ##### Disk Information #####
  variables.disk.device = "/dev/sda";
  ##### Parallels #####
  hardware.parallels = {
    enable = true;
    package = pkgs.linuxKernel.packages.${kernelPackage}.prl-tools;
  };
  boot.kernelPackages = pkgs.${linuxPackage};
  ##### Boot Configuration #####
  boot.initrd.availableKernelModules = [
    "xhci_pci"
    "sr_mod"
  ];
  ##### Security #####
  security.sudo.wheelNeedsPassword = false;
  ##### Packages #####
  environment.systemPackages = with pkgs; [
      linuxKernel.packages.${kernelPackage}.prl-tools
  ];
}