{ lib, ... }:
{
  imports = [
    ../../../../nix/hosts/x86_64/FRACTAL-NORTH/FRACTAL-NORTH.nix
    ../../vm-adapter.nix
  ];
}
