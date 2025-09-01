{ pkgs, lib, ... }:
let
  kernelVersion = "6_16";
  kernelPackage = "linux_${kernelVersion}";
  linuxPackage = "linuxPackages_${kernelVersion}";
in
{
  ##### Host Name #####
  networking.hostName = "VM";
  ##### Disk Information #####
  settings.disk.device = "/dev/sda";
  settings.disk.encryption.enable = false;
  settings.disk.swap.enable = false;
  settings.disk.immutability.enable = true;
  ##### Desktop #####
  settings.desktop.environment = lib.mkForce "plasma-wayland";
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
  ##### Packages #####
  environment.systemPackages = with pkgs; [
      linuxKernel.packages.${kernelPackage}.prl-tools
  ];
  ##### Security #####
  security.sudo.wheelNeedsPassword = false;
  settings.user.admin.autoLogin.enable = true;
  settings.user.admin.autoLock.enable = false;
  ##### Networking #####
  settings.networking.lanSubnet = "10.0.2.3/24";
}