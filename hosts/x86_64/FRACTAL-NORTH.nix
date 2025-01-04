{ config, pkgs, ... }:
{  
  ##### Host Name #####
  networking.hostName = "FRACTAL-NORTH";
  # Disk information
  variables.disk.device = "/dev/nvme0n1"; # This line must exist, but feel free to change the location
  variables.disk.swapSize = "65G";
  # Dual GPU drivers
  services.xserver.videoDrivers = [ "amdgpu" ];
  # AMD drivers
  boot.kernelParams = [ "amdgpu.dc=1" ];
}