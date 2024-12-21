{
  description = "My NixOS System Configuration";
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.11";
    disko = { url = "github:nix-community/disko/v1.10.0"; inputs.nixpkgs.follows = "nixpkgs"; };
    lanzaboote = { url = "github:nix-community/lanzaboote/v0.4.1"; inputs.nixpkgs.follows = "nixpkgs"; };
  };
  outputs = { self, nixpkgs, home-manager, ... }@attrs:
  let
    mkNixosSystem = hostname: nixpkgs.lib.nixosSystem {
      specialArgs = attrs;
      modules = [
        ./configuration.nix
        ./hosts/${hostname}.nix
      ];
    };
  in {
    nixosConfigurations = {
      "MBP-M1-VM" = mkNixosSystem "MBP-M1-VM";
      "FRACTAL-NORTH" = mkNixosSystem "FRACTAL-NORTH";
    };
  };
}