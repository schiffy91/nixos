{ disko, lanzaboote, ... }:

{
  imports = [
    lanzaboote.nixosModules.lanzaboote
    disko.nixosModules.default
    ./modules/shared.nix
    ./modules/partition.nix
    ./modules/boot.nix
    ./modules/graphics.nix
    ./modules/locale.nix
    ./modules/packages.nix
    ./modules/networking.nix
    ./modules/sound.nix
    ./modules/users.nix
  ];
  nix.settings = {
    warn-dirty = false;
    experimental-features = [ "nix-command" "flakes" ];
  };
  system.stateVersion = "24.11";
}