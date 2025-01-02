{ pkgs, ... }:
let
  kernel_version = "6_6";
  kernelPackage = "linux_${kernel_version}";
  linuxPackage = "linuxPackages_${kernel_version}";
in # Parallels Tools is broken on anything greater than 6.11. SMH
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
  ##### X11 Configuration #####
  services.xserver = {
    enable = true;
    displayManager = {
      sddm.wayland.enable = false;  # Force X11
    };
  };
  boot.kernelPackages = pkgs.${linuxPackage};
  ##### Boot Configuration #####
  boot.initrd.availableKernelModules = [
    "xhci_pci"
    "sr_mod"
  ];
  ##### Clipboard Sharing #####
  systemd.user.services.prl-clipboard = {
    description = "Parallels Tools Clipboard Service";
    wantedBy = [ "graphical-session.target" ];
    partOf = [ "graphical-session.target" ];
    serviceConfig = {
      ExecStart = "${pkgs.linuxKernel.packages.${kernelPackage}.prl-tools}/bin/prlcc";
      Restart = "always";
    };
  };
  ##### Packages #####
  environment.systemPackages = with pkgs; [
      linuxKernel.packages.${kernelPackage}.prl-tools
  ];
}