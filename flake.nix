{
  inputs = {
    nixpkgs = { url = "github:NixOS/nixpkgs/nixos-24.11"; };
    nixpkgs-unstable = { url = "github:nixos/nixpkgs/nixos-unstable"; };
    disko = { url = "github:nix-community/disko/v1.10.0"; inputs.nixpkgs.follows = "nixpkgs"; };
    home-manager = { url = "github:nix-community/home-manager/release-24.11"; inputs.nixpkgs.follows = "nixpkgs"; };
    plasma-manager = { url = "github:nix-community/plasma-manager"; inputs.nixpkgs.follows = "nixpkgs"; inputs.home-manager.follows = "home-manager"; };
    inputs.hyprland = { url = "github:hyprwm/Hyprland"; }; #TODO HDR support was added to hyprland at HEAD; switch to nixpkgs-unstable after a new release.
    sbctl-pkg = { url = "github:NixOS/nixpkgs/93dc9803a1ee435e590b02cde9589038d5cc3a4e"; }; #TODO nixpkgs currently uses v0.14; update to v0.16 after the package is updated in nixpkgs.
    lanzaboote = { url = "github:nix-community/lanzaboote/93e6f0d77548be8757c11ebda5c4235ef4f3bc67"; inputs.nixpkgs.follows = "nixpkgs"; }; #TODO Lanzaboote is not in nixpgs, and it's latest release version uses sbctl v0.14; update to a stable release after its pushed to github.
  };
  outputs = inputs@{ self, ... }:
    let
      lib = inputs.nixpkgs.lib;
      mkNixosSystem = hostFile: target: 
        let 
          system = "${baseNameOf (dirOf hostFile)}-linux"; 
          systemModules = [./modules/settings.nix hostFile ] ++
            (if target == "Disk-Operation" then [ ./modules/system/disk.nix ] 
            else (lib.filter (path: lib.hasSuffix ".nix" path) (lib.filesystem.listFilesRecursive ./modules/system)) ++ [{ settings.boot.method = lib.mkForce target; }]); in
        (lib.nixosSystem {
          inherit system;
          specialArgs = { inherit self inputs; unstable-pkgs = import inputs.nixpkgs-unstable { inherit system; config.allowUnfree = true; }; };
          modules = [{
            imports =  systemModules;
            config = {
              nix.settings = {
                experimental-features = [ "nix-command" "flakes" ];
                substituters = ["https://hyprland.cachix.org"]; trusted-public-keys = ["hyprland.cachix.org-1:a7pgxzMz7+chwVL3/pzj6jIBMioiJM7ypFP8PwtkuGc="]; # https://wiki.hyprland.org/Nix/Cachix/
              };
              nixpkgs.config.allowUnfree = true;
              system.stateVersion = "24.11";
            };
          }];
        });
    in { 
      nixosConfigurations = lib.listToAttrs (lib.concatMap (hostFile:
        let name = lib.removeSuffix ".nix" (baseNameOf hostFile); in
          [ 
            { name = "${name}-Disk-Operation"; value = mkNixosSystem hostFile "Disk-Operation"; }
            { name = "${name}-Standard"; value = mkNixosSystem hostFile "Standard"; }
            { name = "${name}-Secure-Boot"; value = mkNixosSystem hostFile "Secure-Boot"; }
          ]
      ) (lib.filter (path: (lib.hasSuffix ".nix" path)) (lib.filesystem.listFilesRecursive ./modules/hosts)));
    };
}
