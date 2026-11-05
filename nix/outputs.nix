# Every flake output. flake.nix stays a manifest of inputs.
{ inputs, self }:
let
  lib = inputs.nixpkgs.lib;
  bootableTargets = [ "Standard-Boot" "Secure-Boot" ];
  diskOperationTargets = [ "Disk-Operation" ];
  isHostEntry = path: lib.hasSuffix ".nix" path && (lib.removeSuffix ".nix" (baseNameOf path)) == (baseNameOf (dirOf path));
  hostFiles = lib.filter isHostEntry (lib.filesystem.listFilesRecursive ./hosts);
  testHostFiles = lib.optionals (builtins.pathExists ../tests/hosts)
    (lib.filter isHostEntry (lib.filesystem.listFilesRecursive ../tests/hosts));
  allHostFiles = hostFiles ++ testHostFiles;
  hostName = hostFile: lib.removeSuffix ".nix" (baseNameOf hostFile);
  hostSystem = hostFile: "${baseNameOf (dirOf (dirOf hostFile))}-linux";
  hostArchitecture = hostFile: baseNameOf (dirOf (dirOf hostFile));
  hostNames = map hostName allHostFiles;
  duplicateHostName = name: (lib.length (lib.filter (candidate: candidate == name) hostNames)) > 1;
  hostOutputName = hostFile:
    let name = hostName hostFile;
    in if duplicateHostName name then "${name}-${hostArchitecture hostFile}" else name;
  systems = lib.unique (map hostSystem allHostFiles);
  systemModuleFiles = lib.mapAttrsToList (name: _: ./system/nixos + "/${name}")  # subdirectories hold module assets, not modules
    (lib.filterAttrs (name: type: type == "regular" && lib.hasSuffix ".nix" name) (builtins.readDir ./system/nixos));
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
        imports = [ ./settings.nix hostFile { settings.boot.method = lib.mkForce target; } ] ++ modules;
        config = baseConfig;
      }];
    };
  mkNixosSystem = hostFile: target:
    let name = hostOutputName hostFile;
  in {
      name = "${name}-${target}";
      value = mkSystem hostFile target systemModuleFiles;
    };
  mkDiskoSystem = hostFile: target: mkSystem hostFile target [ ./system/nixos/disk.nix ];
  mkDiskoConfiguration = hostFile: target:
    let
      name = hostOutputName hostFile;
      nixos = mkDiskoSystem hostFile target;
    in {
      name = "${name}-${target}";
      value.disko.devices = nixos.config.disko.devices;
    };
  flakeSources = depth: attrs: lib.unique (lib.concatMap (input:
    lib.optional (input ? outPath) "${input.outPath}"
    ++ lib.optionals (depth > 0 && input ? inputs) (flakeSources (depth - 1) input.inputs))
    (lib.attrValues attrs));
  mkDiskoCheck = import ../tests/checks/disko_contract.nix { inherit lib mkDiskoSystem; };
  installers = import ./system/nixos/images/installers.nix {
    inherit lib inputs self hostSystem hostOutputName mkSystem systemModuleFiles bootableTargets flakeSources;
    hostFiles = allHostFiles;
    keyHostFiles = hostFiles;
  };
  inherit (installers) mkInstaller installerPackages;
  mkInitrdUnitsCheck = import ../tests/checks/initrd_units.nix { inherit lib; };
  mkUnlockAgentCheck = import ../tests/checks/unlock_agent.nix { inherit lib; };
  mkQuietBootCheck = import ../tests/checks/quiet_boot.nix { inherit lib; };
  mkOwnKeysCheck = import ../tests/checks/own_keys_only.nix { inherit lib; };
  mkHomeTmpfilesCheck = import ../tests/checks/home_tmpfiles.nix { inherit lib; };
  mkInstallerCheck = import ../tests/checks/installer_contract.nix {
    inherit lib mkInstaller bootableTargets hostOutputName;
    prebuiltSources = flakeSources 3 inputs;
  };
  darwinFiles = lib.filter (path: baseNameOf path == "darwin.nix") (lib.filesystem.listFilesRecursive ./hosts);
  darwinName = darwinFile: baseNameOf (dirOf darwinFile);
  darwinConfigurationOf = darwinFile: self.darwinConfigurations.${darwinName darwinFile};
  darwinSystemOf = darwinFile: "${baseNameOf (dirOf (dirOf darwinFile))}-darwin";
  darwinHomeFiles = lib.filter (darwinFile: builtins.pathExists ((dirOf darwinFile) + "/home.nix")) darwinFiles;
  darwinHomeName = darwinFile: "${(darwinConfigurationOf darwinFile).config.settings.users.admin.username}@${darwinName darwinFile}";
  mkDarwinSystem = darwinFile: inputs.nix-darwin.lib.darwinSystem {
    system = darwinSystemOf darwinFile;
    specialArgs = { inherit inputs; };
    modules = [ ./settings.nix ./system/darwin darwinFile ];
  };
  mkDarwinHome = darwinFile: inputs.home-manager-darwin.lib.homeManagerConfiguration {
    inherit (darwinConfigurationOf darwinFile) pkgs;
    extraSpecialArgs = { inherit ((darwinConfigurationOf darwinFile).config) settings; };
    modules = [ ((dirOf darwinFile) + "/home.nix") ];
  };
  in {
  darwinConfigurations = lib.listToAttrs (map (darwinFile: { name = darwinName darwinFile; value = mkDarwinSystem darwinFile; }) darwinFiles);
  homeConfigurations = lib.listToAttrs (map (darwinFile: { name = darwinHomeName darwinFile; value = mkDarwinHome darwinFile; }) darwinHomeFiles);
  packages = lib.genAttrs systems installerPackages;
  diskoConfigurations = lib.listToAttrs (lib.concatMap (hostFile: map (target: mkDiskoConfiguration hostFile target) diskOperationTargets) allHostFiles);
  nixosConfigurations = lib.listToAttrs (lib.concatMap (hostFile: map (target: mkNixosSystem hostFile target) bootableTargets) allHostFiles);
  checks = (lib.genAttrs systems (system:
    let
      pkgs = import inputs.nixpkgs { inherit system; config.allowUnfree = true; };
      systemHosts = lib.filter (hostFile: hostSystem hostFile == system) hostFiles;
    in lib.listToAttrs (lib.concatMap (hostFile: map (target: mkDiskoCheck { inherit pkgs hostFile target; name = "${hostOutputName hostFile}-${target}"; }) diskOperationTargets) systemHosts)
      // (let
        configurations = lib.concatMap (hostFile: map (target: {
          name = "${hostOutputName hostFile}-${target}";
          configuration = mkSystem hostFile target systemModuleFiles;
        }) bootableTargets) systemHosts;
        hostChecks = map (mkCheck: mkCheck { inherit pkgs system configurations; })
          [ mkHomeTmpfilesCheck mkOwnKeysCheck mkQuietBootCheck mkInitrdUnitsCheck mkUnlockAgentCheck ];
        installerCheck =
          let prebuiltHost = lib.findFirst (candidate: hostSystem candidate == system) (lib.head hostFiles) hostFiles;
          in mkInstallerCheck {
            inherit pkgs system prebuiltHost;
            touchHost = lib.findFirst (candidate:
              (mkSystem candidate (lib.head bootableTargets) systemModuleFiles).config.boot.initrd.unl0kr.enable or false)
              null systemHosts;
          };
      in lib.listToAttrs (map (check: { inherit (check) name value; }) (hostChecks ++ [ installerCheck ])))))
    // lib.genAttrs (lib.unique (map darwinSystemOf darwinHomeFiles)) (system: lib.listToAttrs
      (map (darwinFile: { name = "${darwinName darwinFile}-home"; value = self.homeConfigurations.${darwinHomeName darwinFile}.activationPackage; })
        (lib.filter (darwinFile: darwinSystemOf darwinFile == system) darwinHomeFiles)));
  }
