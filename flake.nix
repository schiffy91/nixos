{
  inputs = {
    nixpkgs-darwin = { url = "github:NixOS/nixpkgs/nixpkgs-unstable"; };
    home-manager-darwin = { url = "github:nix-community/home-manager"; inputs.nixpkgs.follows = "nixpkgs-darwin"; };
    nix-darwin = { url = "github:nix-darwin/nix-darwin/master"; inputs.nixpkgs.follows = "nixpkgs-darwin"; };
    nixpkgs = { url = "github:NixOS/nixpkgs/nixos-unstable"; };
    nixpkgs-unstable = { url = "github:nixos/nixpkgs/nixos-unstable"; };
    home-manager = { url = "github:nix-community/home-manager"; inputs.nixpkgs.follows = "nixpkgs-unstable"; };
    plasma-manager = { url = "github:nix-community/plasma-manager"; inputs.nixpkgs.follows = "nixpkgs-unstable"; inputs.home-manager.follows = "home-manager"; };
    disko = { url = "github:nix-community/disko"; inputs.nixpkgs.follows = "nixpkgs-unstable"; };
    lanzaboote = { url = "github:nix-community/lanzaboote"; inputs.nixpkgs.follows = "nixpkgs-unstable"; };
    jovian = { url = "github:Jovian-Experiments/Jovian-NixOS"; inputs.nixpkgs.follows = "nixpkgs"; };
    btrc = { url = "github:schiffy91/btrc"; inputs.nixpkgs.follows = "nixpkgs-unstable"; };
    semu = { url = "github:schiffy91/semu"; inputs.nixpkgs.follows = "nixpkgs-unstable"; inputs.btrc.follows = "btrc"; };
    logitech-trueforce = { url = "github:mescon/logitech-trueforce-linux-driver"; inputs.nixpkgs.follows = "nixpkgs"; };
  };
  outputs = inputs@{ self, ... }:
    let
      lib = inputs.nixpkgs.lib;
      bootableTargets = [ "Standard-Boot" "Secure-Boot" ];
      diskOperationTargets = [ "Disk-Operation" ];
      isHostEntry = path: lib.hasSuffix ".nix" path && (lib.removeSuffix ".nix" (baseNameOf path)) == (baseNameOf (dirOf path));
      hostFiles = lib.filter isHostEntry (lib.filesystem.listFilesRecursive ./nix/hosts);
      hostName = hostFile: lib.removeSuffix ".nix" (baseNameOf hostFile);
      hostSystem = hostFile: "${baseNameOf (dirOf (dirOf hostFile))}-linux";
      hostArchitecture = hostFile: baseNameOf (dirOf (dirOf hostFile));
      hostNames = map hostName hostFiles;
      duplicateHostName = name: (lib.length (lib.filter (candidate: candidate == name) hostNames)) > 1;
      hostOutputName = hostFile:
        let name = hostName hostFile;
        in if duplicateHostName name then "${name}-${hostArchitecture hostFile}" else name;
      systems = lib.unique (map hostSystem hostFiles);
      systemModuleFiles = lib.mapAttrsToList (name: _: ./nix/system/nixos + "/${name}")  # subdirectories hold module assets, not modules
        (lib.filterAttrs (name: type: type == "regular" && lib.hasSuffix ".nix" name) (builtins.readDir ./nix/system/nixos));
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
        let name = hostOutputName hostFile;
        in {
          name = "${name}-${target}";
          value = mkSystem hostFile target systemModuleFiles;
        };
      mkDiskoSystem = hostFile: target: mkSystem hostFile target [ ./nix/system/nixos/disk.nix ];
      mkDiskoConfiguration = hostFile: target:
        let
          name = hostOutputName hostFile;
          nixos = mkDiskoSystem hostFile target;
        in {
          name = "${name}-${target}";
          value.disko.devices = nixos.config.disko.devices;
        };
      mkDiskoCheck = import ./tests/checks/disko_contract.nix { inherit lib mkDiskoSystem; };
      # One installer ISO per architecture, taking the admin keys from the host
      # config of that architecture so nothing is written down twice.
      # The ISO's whole promise is no typing: keys already authorized, this
      # repository already at a writable /etc/nixos, nixosctl already built.
      mkInstallerCheck = system:
        let
          cfg = (mkInstaller system).config;
          pkgs = import inputs.nixpkgs { inherit system; config.allowUnfree = true; };
          repositoryScript = cfg.systemd.services.installer-repository.script or "";
          failures = lib.optional (cfg.users.users.root.openssh.authorizedKeys.keys == []) "root has no authorized keys"
            ++ lib.optional (!(cfg.systemd.services ? installer-repository)) "no service places /etc/nixos"
            ++ lib.optional (!(lib.hasInfix "--chmod=u+w" repositoryScript)) "/etc/nixos would not be writable"
            ++ lib.optional (!(lib.any (p: (p.name or "") == "nixosctl") cfg.environment.systemPackages)) "nixosctl is not installed";
        in pkgs.runCommand "installer-contract-${system}" { } (
          if failures == [] then "touch $out"
          else "echo ${lib.escapeShellArg (lib.concatStringsSep "; " failures)} >&2; exit 1");
      mkInstaller = system:
        let hostFile = lib.findFirst (candidate: hostSystem candidate == system) (lib.head hostFiles) hostFiles;
            host = mkSystem hostFile (lib.head bootableTargets) systemModuleFiles;
        in import ./nix/system/nixos/images/installer.nix {
          inherit inputs self system;
          authorizedKeys = host.config.settings.users.admin.authorizedKeys;
          stateVersion = host.config.system.stateVersion;
        };
      darwinFiles = lib.filter (path: baseNameOf path == "darwin.nix") (lib.filesystem.listFilesRecursive ./nix/hosts);
      darwinName = darwinFile: baseNameOf (dirOf darwinFile);
      darwinConfigurationOf = darwinFile: self.darwinConfigurations.${darwinName darwinFile};
      darwinSystemOf = darwinFile: "${baseNameOf (dirOf (dirOf darwinFile))}-darwin";
      darwinHomeFiles = lib.filter (darwinFile: builtins.pathExists ((dirOf darwinFile) + "/home.nix")) darwinFiles;
      darwinHomeName = darwinFile: "${(darwinConfigurationOf darwinFile).config.settings.users.admin.username}@${darwinName darwinFile}";
      mkDarwinSystem = darwinFile: inputs.nix-darwin.lib.darwinSystem {
        system = darwinSystemOf darwinFile;
        specialArgs = { inherit inputs; };
        modules = [ ./nix/settings.nix ./nix/system/darwin darwinFile ];
      };
      mkDarwinHome = darwinFile: inputs.home-manager-darwin.lib.homeManagerConfiguration {
        inherit (darwinConfigurationOf darwinFile) pkgs;
        extraSpecialArgs = { inherit ((darwinConfigurationOf darwinFile).config) settings; };
        modules = [ ((dirOf darwinFile) + "/home.nix") ];
      };
    in {
      darwinConfigurations = lib.listToAttrs (map (darwinFile: { name = darwinName darwinFile; value = mkDarwinSystem darwinFile; }) darwinFiles);
      homeConfigurations = lib.listToAttrs (map (darwinFile: { name = darwinHomeName darwinFile; value = mkDarwinHome darwinFile; }) darwinHomeFiles);
      packages = lib.genAttrs systems (system: { installer = (mkInstaller system).config.system.build.isoImage; });
      diskoConfigurations = lib.listToAttrs (lib.concatMap (hostFile: map (target: mkDiskoConfiguration hostFile target) diskOperationTargets) hostFiles);
      nixosConfigurations = lib.listToAttrs (lib.concatMap (hostFile: map (target: mkNixosSystem hostFile target) bootableTargets) hostFiles);
      checks = (lib.genAttrs systems (system:
        let
          pkgs = import inputs.nixpkgs { inherit system; config.allowUnfree = true; };
          systemHosts = lib.filter (hostFile: hostSystem hostFile == system) hostFiles;
        in lib.listToAttrs (lib.concatMap (hostFile: map (target: mkDiskoCheck { inherit pkgs hostFile target; name = "${hostOutputName hostFile}-${target}"; }) diskOperationTargets) systemHosts)
          // { installer-contract = mkInstallerCheck system; }))
        // lib.genAttrs (lib.unique (map darwinSystemOf darwinHomeFiles)) (system: lib.listToAttrs
          (map (darwinFile: { name = "${darwinName darwinFile}-home"; value = self.homeConfigurations.${darwinHomeName darwinFile}.activationPackage; })
            (lib.filter (darwinFile: darwinSystemOf darwinFile == system) darwinHomeFiles)));
    };
}
