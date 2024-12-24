{
  description = "My NixOS System Configuration";
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.11";
    disko = { url = "github:nix-community/disko/v1.10.0"; inputs.nixpkgs.follows = "nixpkgs"; };
    lanzaboote = { url = "github:nix-community/lanzaboote/v0.4.1"; inputs.nixpkgs.follows = "nixpkgs"; };
  };
  outputs = inputs@{ self, nixpkgs, disko, lanzaboote, ... }:
    let
      hosts = [
        { name = "MBP-M1-VM"; system = "aarch64-linux"; }
        { name = "FRACTAL-NORTH"; system = "x86_64-linux"; }
      ];
      modules = { 
        standard = [ ./configuration.nix ];
        disko  = [ disko.nixosModules.disko ./modules/disk.nix ]; 
      };
      mkSystem = host: modules: nixpkgs.lib.nixosSystem {
        system = host.system;
        specialArgs = { inherit self inputs; };
        modules = modules ++ [ { networking.hostName = host.name; } ./hosts/${host.name}.nix ];
      };
      configurations = nixpkgs.lib.concatMap (host:[
        { name = host.name; system = mkSystem host modules.standard; }
        { name = "${host.name}-DISKO"; system = mkSystem host modules.disko; }
      ]) hosts;
    in { 
      nixosConfigurations = nixpkgs.lib.listToAttrs (map (configuration: { 
        name = configuration.name; 
        value = configuration.system; 
      }) configurations); 
    };
}
