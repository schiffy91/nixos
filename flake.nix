{
  description = "My NixOS System Configuration";
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.11";
    disko = { url = "github:nix-community/disko/v1.10.0"; inputs.nixpkgs.follows = "nixpkgs"; };
    ghostty = { url = "github:ghostty-org/ghostty"; };
    #TODO nixpkgs currently uses v0.14; update to v0.16 after the package is updated in nixpkgs.
    sbctl-pkg.url = "github:NixOS/nixpkgs/93dc9803a1ee435e590b02cde9589038d5cc3a4e";
    #TODO Lanzaboote is not in nixpgs, and it's latest release version uses sbctl v0.14; update to a stable release after its pushed to github.
    lanzaboote = { url = "github:nix-community/lanzaboote/93e6f0d77548be8757c11ebda5c4235ef4f3bc67"; inputs.nixpkgs.follows = "nixpkgs"; };
  };
  outputs = inputs@{ self, nixpkgs, ... }:
    let
      lib = nixpkgs.lib;
      hostFiles = ((lib.filter (path: (lib.hasSuffix ".nix" path))) (lib.filesystem.listFilesRecursive ./hosts));
      getHostName = hostFile: (lib.removeSuffix ".nix" (baseNameOf hostFile));
      getArchitecture = hostFile: (baseNameOf (dirOf hostFile));
      mkTarget = hostFile: modules: 
        (lib.nixosSystem {
          specialArgs = { inherit self inputs; };
          system = "${getArchitecture hostFile}-linux";
          modules = [ ./variables.nix ] ++ modules ++ [ hostFile { networking.hostName = lib.mkForce (getHostName hostFile); } ];
        });
    in { 
      nixosConfigurations = lib.listToAttrs (lib.concatMap (hostFile:
        let name = getHostName hostFile; in 
        [ 
          { name = "${name}-mount"; value = mkTarget hostFile [ ./modules/disk.nix ]; }
          { name = "${name}-standard"; value = mkTarget hostFile [ ./configuration.nix ]; }
          { name = "${name}-secure-boot"; value = mkTarget hostFile [ ./configuration.nix ({ lib, ... }: { boot.lanzaboote.enable = lib.mkForce true; }) ]; }
        ]
      ) hostFiles);
    };
}
