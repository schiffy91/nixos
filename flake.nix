{
  inputs = {
    nixpkgs = { url = "github:NixOS/nixpkgs/nixos-24.11"; };
    nixpkgs-unstable = { url = "github:nixos/nixpkgs/nixos-unstable"; };
    home-manager = { url = "github:nix-community/home-manager/release-24.11"; inputs.nixpkgs.follows = "nixpkgs-unstable"; };
    plasma-manager = { url = "github:nix-community/plasma-manager"; inputs.nixpkgs.follows = "nixpkgs-unstable"; inputs.home-manager.follows = "home-manager"; };
    disko = { url = "github:nix-community/disko"; inputs.nixpkgs.follows = "nixpkgs-unstable"; };
    lanzaboote = { url = "github:nix-community/lanzaboote/a65905a09e2c43ff63be8c0e86a93712361f871e"; inputs.nixpkgs.follows = "nixpkgs"; }; #TODO Lanzaboote is not in nixpgs, and it's latest release version uses sbctl v0.14; update to a stable release after its pushed to github.
  };
  outputs = inputs@{ self, ... }:
    let
      lib = inputs.nixpkgs.lib;
      mkNixosSystem = hostFile: target: 
        let
          name = lib.removeSuffix ".nix" (baseNameOf hostFile);
          system = "${baseNameOf (dirOf hostFile)}-linux"; 
          targetModules = (
            if lib.hasInfix "Boot" target then 
              [{ settings.boot.method = lib.mkForce target; }] ++ lib.filter (path: lib.hasSuffix ".nix" path) (lib.filesystem.listFilesRecursive ./modules/system)
            else
              [ ./modules/system/disk.nix ]
          ); 
          pkgs-unstable = import inputs.nixpkgs-unstable { inherit system; config.allowUnfree = true; };
        in {
          name = "${name}-${target}";
          value = lib.nixosSystem {
            inherit system;
            specialArgs = { inherit self inputs pkgs-unstable; };
            modules = [{
              imports = [ ./modules/settings.nix hostFile ] ++ targetModules;
              config = {
                nix = {
                  channel.enable = false;
                  settings.experimental-features = [ "nix-command" "flakes" ];
                };
                nixpkgs.config.allowUnfree = true;
                system.stateVersion = "24.11";
              };
            }];
          };
        };
    in { 
      nixosConfigurations = lib.listToAttrs (
        lib.concatMap (hostFile: [ 
          (mkNixosSystem hostFile "Disk-Operation")
          (mkNixosSystem hostFile "Standard-Boot")
          (mkNixosSystem hostFile "Secure-Boot")
        ]) (lib.filter (path: (lib.hasSuffix ".nix" path)) (lib.filesystem.listFilesRecursive ./modules/hosts))
      );
    };
}
