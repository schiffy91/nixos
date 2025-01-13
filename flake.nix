{
  inputs = {
    nixpkgs = { url = "github:NixOS/nixpkgs/nixos-24.11"; };
    nixpkgs-unstable = { url = "github:nixos/nixpkgs/nixos-unstable"; };
    home-manager = { url = "github:nix-community/home-manager/release-24.11"; inputs.nixpkgs.follows = "nixpkgs"; };
    plasma-manager = { url = "github:nix-community/plasma-manager"; inputs.nixpkgs.follows = "nixpkgs"; inputs.home-manager.follows = "home-manager"; };
    #hyprland = { url = "github:hyprwm/Hyprland"; }; #TODO HDR support was added to hyprland at HEAD; switch to nixpkgs-unstable after a new release.
    disko = { url = "github:nix-community/disko/v1.10.0"; inputs.nixpkgs.follows = "nixpkgs"; };
    impermanence = { url = "github:nix-community/impermanence"; };
    lanzaboote = { url = "github:nix-community/lanzaboote/93e6f0d77548be8757c11ebda5c4235ef4f3bc67"; inputs.nixpkgs.follows = "nixpkgs"; }; #TODO Lanzaboote is not in nixpgs, and it's latest release version uses sbctl v0.14; update to a stable release after its pushed to github.
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
          unstable-pkgs = import inputs.nixpkgs-unstable { inherit system; config.allowUnfree = true; };
        in {
          name = "${name}-${target}";
          value = lib.nixosSystem {
            inherit system;
            specialArgs = { inherit self inputs unstable-pkgs; };
            modules = [{
              imports = [ ./modules/settings.nix hostFile ] ++ targetModules;
              config = {
                nix.channel.enable = false;
                nix.settings.experimental-features = [ "nix-command" "flakes" ];
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
