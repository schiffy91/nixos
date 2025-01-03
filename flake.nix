{
  description = "My NixOS System Configuration";
  inputs = {
    nixpkgs = { url = "github:NixOS/nixpkgs/nixos-24.11"; };
    nixpkgs-unstable = { url = "github:nixos/nixpkgs/nixos-unstable"; };
    disko = { url = "github:nix-community/disko/v1.10.0"; inputs.nixpkgs.follows = "nixpkgs"; };
    #TODO nixpkgs currently uses v0.14; update to v0.16 after the package is updated in nixpkgs.
    sbctl-pkg = { url = "github:NixOS/nixpkgs/93dc9803a1ee435e590b02cde9589038d5cc3a4e"; };
    #TODO Lanzaboote is not in nixpgs, and it's latest release version uses sbctl v0.14; update to a stable release after its pushed to github.
    lanzaboote = { url = "github:nix-community/lanzaboote/93e6f0d77548be8757c11ebda5c4235ef4f3bc67"; inputs.nixpkgs.follows = "nixpkgs"; };
    home-manager = { url = "github:nix-community/home-manager"; inputs.nixpkgs.follows = "nixpkgs-unstable"; };
    plasma-manager = { url = "github:nix-community/plasma-manager"; inputs.nixpkgs.follows = "nixpkgs-unstable"; inputs.home-manager.follows = "home-manager"; };
  };
  outputs = inputs@{ self, nixpkgs, home-manager, plasma-manager, ... }:
    let
      lib = nixpkgs.lib;
      getNixPathsIn = directory: ((lib.filter (path: (lib.hasSuffix ".nix" path))) (lib.filesystem.listFilesRecursive directory));
      # Optional modules
      lanzabooteModule = ({ lib, ... }: { boot.lanzaboote.enable = lib.mkForce true; });
      homeManagerModule = home-manager.nixosModules.home-manager {
        home-manager.useGlobalPkgs = true;
        home-manager.useUserPackages = true;
        home-manager.sharedModules = [ plasma-manager.homeManagerModules.plasma-manager ];
        home-manager.users = lib.listToAttrs (lib.map (path: { name = lib.removeSuffix ".nix" (baseNameOf path); value = import path; }) (getNixPathsIn ./users));
      };
      mkTarget = hostFile: modules: 
        (lib.nixosSystem {
          specialArgs = { inherit self inputs; };
          system = "${baseNameOf (dirOf hostFile)}-linux";
          modules = [ ./variables.nix hostFile ] ++ modules;
        });
    in { 
      nixosConfigurations = lib.listToAttrs (lib.concatMap (hostFile:
        let name = lib.removeSuffix ".nix" (baseNameOf hostFile); in 
        [ 
          { name = "${name}-Disk-Operation"; value = mkTarget hostFile [ ./system/disk.nix ]; }
          { name = "${name}-Standard"; value = mkTarget hostFile [ ./configuration.nix homeManagerModule ]; }
          { name = "${name}-Secure-Boot"; value = mkTarget hostFile [ ./configuration.nix homeManagerModule lanzabooteModule ]; }
        ]
      ) (getNixPathsIn ./hosts));
    };
}
