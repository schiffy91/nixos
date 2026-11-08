{ lib, ... }:
{
  imports = [
    ../../../../config/hosts/x86_64/FRACTAL-NORTH/default.nix
    ../../vm-adapter.nix
  ];
}
