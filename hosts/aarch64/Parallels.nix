{ pkgs, ... }:
{
  boot.kernelPackages = pkgs.linuxPackages_6_6; # Parallels Tools is broken on anything greater than 6.6. SMH
  # TODO FIX
}