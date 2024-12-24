{ pkgs, ... }:
{
  # Disk information
  diskOverrides.device = "/dev/sda";
  diskOverrides.swapSize = "17G";

  # VMWare
  virtualisation.vmware.guest.enable = true;
  # ARM64 Packages
  environment.systemPackages = with pkgs; [
    chromium
  ];
}