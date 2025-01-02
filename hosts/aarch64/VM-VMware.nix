{ pkgs, ... }:
{
  ##### Host Name #####
  networking.hostName = "VM";
  ##### Disk Information #####
  variables.disk.device = "/dev/nvme0n1";
  variables.disk.swapSize = "1G"; # Small swap for a VM
  ##### VMware #####
  virtualisation.vmware.guest = {
    enable = true;
    headless = false;  # Set to false to enable GUI tools including copy/paste
    package = pkgs.open-vm-tools;  # Explicitly set the package
  };
  ##### Display Manager #####
  services.xserver.enable = true;
  services.displayManager.sddm = {
    enable = false;
    wayland.enable = false;
  };
  ##### Boot Configuration #####
  boot.initrd.availableKernelModules = [
    "xhci_pci"
    "sr_mod"
    "nvme"
  ];
  boot.kernelModules = [ "kvm-arm" ];
  ##### Packages #####
  environment.systemPackages = with pkgs; [
    open-vm-tools
  ];
  ##### Shared Folders #####
  fileSystems."/mnt/hgfs" = {
    device = ".host:/";
    fsType = "fuse.vmhgfs-fuse";
    options = [ "defaults" "allow_other" ];
  };
}