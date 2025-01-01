{ pkgs, ... }:
{
  ##### Disk Information #####
  variables.disk.device = "/dev/sda";
  variables.disk.swapSize = "1G"; # Small swap for a VM
  ##### Parallels #####
  hardware.parallels.enable = true;
  boot.kernelPackages = pkgs.linuxPackages_6_6; # Parallels Tools is broken on anything greater than 6.6. SMH
}