{ lib, ... }:
{
  imports = [
    ../../../../config/hosts/x86_64/STEAM-DECK-OLED/default.nix
    ../../vm-adapter.nix
  ];
}
