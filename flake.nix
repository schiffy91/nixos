{
  inputs = {
    nixpkgs = { url = "github:NixOS/nixpkgs/nixos-24.11"; };
    nixpkgs-unstable = { url = "github:nixos/nixpkgs/nixos-unstable"; };
    disko = { url = "github:nix-community/disko/v1.10.0"; inputs.nixpkgs.follows = "nixpkgs"; };
    home-manager = { url = "github:nix-community/home-manager/release-24.11"; inputs.nixpkgs.follows = "nixpkgs"; };
    plasma-manager = { url = "github:nix-community/plasma-manager"; inputs.nixpkgs.follows = "nixpkgs"; inputs.home-manager.follows = "home-manager"; };
    sbctl-pkg = { url = "github:NixOS/nixpkgs/93dc9803a1ee435e590b02cde9589038d5cc3a4e"; }; #TODO nixpkgs currently uses v0.14; update to v0.16 after the package is updated in nixpkgs.
    lanzaboote = { url = "github:nix-community/lanzaboote/93e6f0d77548be8757c11ebda5c4235ef4f3bc67"; inputs.nixpkgs.follows = "nixpkgs"; }; #TODO Lanzaboote is not in nixpgs, and it's latest release version uses sbctl v0.14; update to a stable release after its pushed to github.
  };
  outputs = inputs@{ self, ... }:
    let
      lib = inputs.nixpkgs.lib;
      mkTarget = hostFile: modules: 
        let system = "${baseNameOf (dirOf hostFile)}-linux"; in (lib.nixosSystem {
          inherit system;
          specialArgs = { inherit self inputs; unstable-pkgs = import inputs.nixpkgs-unstable { inherit system; config.allowUnfree = true; }; };
          modules = [ { nixpkgs = { config.allowUnfree = true; }; } ./modules/settings.nix hostFile ] ++ modules;
        });
    in { 
      nixosConfigurations = lib.listToAttrs (lib.concatMap (hostFile:
        let 
          name = lib.removeSuffix ".nix" (baseNameOf hostFile); 
          setBootLoader = bootLoader: ({ lib, ...}: { settings.boot.method = lib.mkForce bootLoader; }); in 
          [ 
            { name = "${name}-Disk-Operation"; value = mkTarget hostFile [ ./modules/system/disk.nix ]; }
            { name = "${name}-Standard"; value = mkTarget hostFile [ ./configuration.nix  (setBootLoader "Standard") ]; }
            { name = "${name}-Secure-Boot"; value = mkTarget hostFile [ ./configuration.nix (setBootLoader "Secure-Boot") ]; }
          ]
      ) (lib.filter (path: (lib.hasSuffix ".nix" path)) (lib.filesystem.listFilesRecursive ./hosts)));
    };
}
