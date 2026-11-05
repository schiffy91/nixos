{ lib, ... }:
{
  imports = [
    ../../../../nix/hosts/x86_64/STEAM-DECK-OLED/STEAM-DECK-OLED.nix
    ../../vm-adapter.nix
  ];
}
