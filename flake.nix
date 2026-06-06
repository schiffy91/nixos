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
      mkSystem = hostFile: target: modules:
        let
          system = hostSystem hostFile;
          pkgs-unstable = import inputs.nixpkgs-unstable { inherit system; config.allowUnfree = true; };
        in lib.nixosSystem {
          inherit system;
          specialArgs = { inherit self inputs pkgs-unstable; };
          modules = [{
            imports = [ ./nix/settings.nix hostFile { settings.boot.method = lib.mkForce target; } ] ++ modules;
            config = baseConfig;
          }];
        };
      mkNixosSystem = hostFile: target:
        let
          name = lib.removeSuffix ".nix" (baseNameOf hostFile);
        in {
          name = "${name}-${target}";
          value = mkSystem hostFile target systemModuleFiles;
        };
      mkDiskoSystem = hostFile: target:
        mkSystem hostFile target [ ./nix/system/disk.nix ];
      mkDiskoConfiguration = hostFile: target:
        let
          name = lib.removeSuffix ".nix" (baseNameOf hostFile);
          nixos = mkDiskoSystem hostFile target;
        in {
          name = "${name}-${target}";
          value.disko.devices = nixos.config.disko.devices;
        };
      mkDiskoCheck = pkgs: hostFile:
        let
          target = "Disk-Operation";
          name = "${lib.removeSuffix ".nix" (baseNameOf hostFile)}-${target}";
          nixos = mkDiskoSystem hostFile target;
          settings = nixos.config.settings.disk;
          disk = builtins.getAttr settings.label.main nixos.config.disko.devices.disk;
          partitions = disk.content.partitions;
          boot = builtins.getAttr settings.label.boot partitions;
          root = builtins.getAttr settings.label.root partitions;
          rootContent = root.content;
          hasRecovery = builtins.hasAttr settings.label.recovery partitions;
          recovery =
            if hasRecovery
            then builtins.getAttr settings.label.recovery partitions
            else null;
          btrfs =
            if rootContent.type == "luks"
            then rootContent.content
            else rootContent;
          subvolumeNames = builtins.attrNames btrfs.subvolumes;
          expect = label: actual: expected:
            if actual == expected
            then actual
            else builtins.throw "${name}: expected ${label} to be ${builtins.toJSON expected}, got ${builtins.toJSON actual}";
          expectContains = label: value: values:
            if builtins.elem value values
            then values
            else builtins.throw "${name}: expected ${label} to contain ${builtins.toJSON value}";
          subvolumeContract =
            map (volume: expectContains "root btrfs subvolumes" volume.name subvolumeNames)
              settings.subvolumes.volumes;
          recoveryContract = lib.optionals hasRecovery [
            (expect "recovery.type" recovery.type "EF00")
            (expect "recovery.content.format" recovery.content.format "vfat")
            (expect "recovery.content.mountpoint" recovery.content.mountpoint settings.recovery.mountPoint)
            (expectContains "recovery.content.extraArgs" "-n" recovery.content.extraArgs)
            (expectContains "recovery.content.extraArgs" settings.recovery.filesystemLabel recovery.content.extraArgs)
            (expectContains "recovery.content.mountOptions" "nofail" recovery.content.mountOptions)
          ];
          contract = [
            (expect "disk.type" disk.type "disk")
            (expect "disk.content.type" disk.content.type "gpt")
            (expect "boot.type" boot.type "EF00")
            (expect "boot.content.format" boot.content.format "vfat")
            (expect "boot.content.mountpoint" boot.content.mountpoint settings.boot.efiSysMountPoint)
            (expect "root.size" root.size "100%")
            (expect "root.content.type" rootContent.type (if settings.encryption.enable then "luks" else "btrfs"))
            (expect "root filesystem type" btrfs.type "btrfs")
            (expect "recovery partition enabled" hasRecovery settings.recovery.enable)
          ] ++ lib.optionals settings.encryption.enable [
            (expect "root LUKS name" rootContent.name settings.label.root)
          ] ++ subvolumeContract ++ recoveryContract;
          checked = builtins.deepSeq contract name;
        in {
          name = "disko-${name}";
          value = pkgs.runCommand "check-${name}" { } ''
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
