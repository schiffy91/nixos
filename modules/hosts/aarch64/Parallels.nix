{ pkgs, lib, ... }: {
let
  kernel_version = "6_14";
  kernelPackage = "linux_${kernel_version}";
  linuxPackage = "linuxPackages_${kernel_version}";
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
  settings.desktop.environment = lib.mkForce "gnome-wayland";
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
  settings.networking.lanSubnet = "192.168.64.0/24";
}