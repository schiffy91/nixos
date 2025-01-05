#TODO Clipboard doesn't work
{ pkgs, ... }:
{
  ##### Host Name #####
  networking.hostName = "VM";
  ##### Disk Information #####
  variables.disk.device = "/dev/nvme0n1"; 
  ##### VMware #####
  virtualisation.vmware.guest = {
    enable = true;
    headless = false;
    package = pkgs.open-vm-tools;
  };
  ##### Display Manager #####
  services.xserver.enable = true;
  ##### Boot Configuration #####
  boot.initrd.availableKernelModules = [
    "xhci_pci"
    "sr_mod"
    "nvme"
  ];
  boot.kernelModules = [ "kvm-arm" ];
  ##### Security #####
  security.sudo.wheelNeedsPassword = false;
  ##### Packages #####
  environment.systemPackages = with pkgs; [
    open-vm-tools
  ];
  ##### Networking #####
  #TODO Remove this if my script works. variables.networking.lanSubnet = "192.168.64.0/24";
}