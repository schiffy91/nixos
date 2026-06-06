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
      hostSystem = hostFile: "${baseNameOf (dirOf (dirOf hostFile))}-linux";
      systems = lib.unique (map hostSystem hostFiles);
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
          system = hostSystem hostFile;
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
          system = hostSystem hostFile;
          pkgs-unstable = import inputs.nixpkgs-unstable { inherit system; config.allowUnfree = true; };
          nixos = lib.nixosSystem {
            inherit system;
            specialArgs = { inherit self inputs pkgs-unstable; };
            modules = [{
              imports = [
                ./nix/settings.nix
                hostFile
                { settings.boot.method = lib.mkForce target; }
                ./nix/system/disk.nix
              ];
              config = baseConfig;
            }];
          };
        in {
          name = "${name}-${target}";
          value.disko.devices = nixos.config.disko.devices;
        };
      mkDiskoCheck = pkgs: hostFile:
        let
          disko = mkDiskoConfiguration hostFile "Disk-Operation";
          disk = disko.value.disko.devices.disk.main;
          partitions = disk.content.partitions;
          contract = [
            disk.device
            disk.type
            disk.content.type
            partitions.boot.type
            partitions.root.size
          ] ++ lib.optional (partitions ? recovery) partitions.recovery.type;
          checked = builtins.deepSeq contract disko.name;
        in {
          name = "disko-${disko.name}";
          value = pkgs.runCommand "check-${disko.name}" { } ''
            printf '%s\n' ${lib.escapeShellArg checked} > "$out"
          '';
        };
    in {
      diskoConfigurations = lib.listToAttrs (lib.concatMap (hostFile: map (target: mkDiskoConfiguration hostFile target) diskOperationTargets) hostFiles);
      nixosConfigurations = lib.listToAttrs (lib.concatMap (hostFile: map (target: mkNixosSystem hostFile target) bootableTargets) hostFiles);
      checks = lib.genAttrs systems (system:
        let
          pkgs = import inputs.nixpkgs { inherit system; config.allowUnfree = true; };
          systemHosts = lib.filter (hostFile: hostSystem hostFile == system) hostFiles;
        in lib.listToAttrs (map (mkDiskoCheck pkgs) systemHosts));
    };
}
