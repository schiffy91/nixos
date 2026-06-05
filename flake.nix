{
  inputs = {
    nixpkgs = { url = "github:NixOS/nixpkgs/nixos-unstable"; };
    nixpkgs-unstable = { url = "github:nixos/nixpkgs/nixos-unstable"; };
    home-manager = { url = "github:nix-community/home-manager"; inputs.nixpkgs.follows = "nixpkgs-unstable"; };
    plasma-manager = { url = "github:nix-community/plasma-manager"; inputs.nixpkgs.follows = "nixpkgs-unstable"; inputs.home-manager.follows = "home-manager"; };
    disko = { url = "github:nix-community/disko"; inputs.nixpkgs.follows = "nixpkgs-unstable"; };
    lanzaboote = { url = "github:nix-community/lanzaboote"; inputs.nixpkgs.follows = "nixpkgs-unstable"; };
    btrc = { url = "github:schiffy91/btrc"; inputs.nixpkgs.follows = "nixpkgs-unstable"; };
  };
  outputs = inputs@{ self, ... }:
    let
      lib = inputs.nixpkgs.lib;
      bootableTargets = [ "Standard-Boot" "Secure-Boot" ];
      diskOperationTargets = [ "Disk-Operation" ];
      isHostEntry = path: lib.hasSuffix ".nix" path && (lib.removeSuffix ".nix" (baseNameOf path)) == (baseNameOf (dirOf path));
      hostFiles = lib.filter isHostEntry (lib.filesystem.listFilesRecursive ./nix/hosts);
      systemModuleFiles = lib.filter (path: lib.hasSuffix ".nix" path) (lib.filesystem.listFilesRecursive ./nix/system);
      baseConfig = {
        nix = {
          channel.enable = false;
          settings.experimental-features = [ "nix-command" "flakes" ];
        };
        nixpkgs.config.allowUnfree = true;
        system.stateVersion = "24.11";
      };
      mkNixosSystem = hostFile: target:
        let
          name = lib.removeSuffix ".nix" (baseNameOf hostFile);
          system = "${baseNameOf (dirOf (dirOf hostFile))}-linux";
          pkgs-unstable = import inputs.nixpkgs-unstable { inherit system; config.allowUnfree = true; };
        in {
          name = "${name}-${target}";
          value = lib.nixosSystem {
            inherit system;
            specialArgs = { inherit self inputs pkgs-unstable; };
            modules = [{
              imports = [ ./nix/settings.nix hostFile { settings.boot.method = lib.mkForce target; } ] ++ systemModuleFiles;
              config = baseConfig;
            }];
          };
        };
      mkDiskoConfiguration = hostFile: target:
        let
          name = lib.removeSuffix ".nix" (baseNameOf hostFile);
          system = "${baseNameOf (dirOf (dirOf hostFile))}-linux";
          pkgs-unstable = import inputs.nixpkgs-unstable { inherit system; config.allowUnfree = true; };
          nixos = lib.nixosSystem {
            inherit system;
            specialArgs = { inherit self inputs pkgs-unstable; };
            modules = [{
              imports = [ ./nix/settings.nix hostFile ./nix/system/disk.nix ];
              config = baseConfig;
            }];
          };
        in {
          name = "${name}-${target}";
          value = nixos.config.disko.devices;
        };
    in {
      diskoConfigurations = lib.listToAttrs (lib.concatMap (hostFile: map (target: mkDiskoConfiguration hostFile target) diskOperationTargets) hostFiles);
      nixosConfigurations = lib.listToAttrs (lib.concatMap (hostFile: map (target: mkNixosSystem hostFile target) bootableTargets) hostFiles);
    };
}
