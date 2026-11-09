{ inputs, lib, hosts, publicHosts, systems, diskOperations, installers, homes }:
let
  mkHostLayoutCheck = import ../../tests/checks/host_layout.nix {
    inherit lib hosts publicHosts systems diskOperations homes;
  };
  mkDiskoCheck = import ../../tests/checks/disko_contract.nix {
    inherit lib;
    inherit (diskOperations) mkDiskoSystem;
  };
  mkHostChecks = map (path: lib.callPackageWith { inherit lib inputs; } path {}) [
    ../../tests/checks/home_tmpfiles.nix
    ../../tests/checks/plasma_startup.nix
    ../../tests/checks/own_keys_only.nix
    ../../tests/checks/quiet_boot.nix
    ../../tests/checks/initrd_units.nix
    ../../tests/checks/unlock_agent.nix
    ../../tests/checks/package_groups.nix
    ../../tests/checks/app_hierarchy.nix
    ../../tests/checks/persistence_ownership.nix
  ];
  mkInstallerCheck = import ../../tests/checks/installer_contract.nix {
    inherit lib;
    inherit (installers) mkInstaller;
    inherit (systems) bootableTargets;
    inherit (hosts) hostOutputName;
    prebuiltSources = installers.flakeSources 3 inputs;
  };
in lib.genAttrs hosts.systems (system:
  let
    pkgs = import inputs.nixpkgs { inherit system; config.allowUnfree = true; };
    systemHosts = lib.filter (host: host.system == system) hosts.physical;
    configurations = lib.concatMap (host: map (target: {
      name = "${hosts.hostOutputName host.module}-${target}";
      configuration = systems.mkSystem host.module target systems.systemModuleFiles;
    }) systems.bootableTargets) systemHosts;
    diskChecks = lib.concatMap (host: map (target: mkDiskoCheck {
      inherit pkgs target;
      hostFile = host.module;
      name = "${hosts.hostOutputName host.module}-${target}";
    }) diskOperations.targets) systemHosts;
    hostChecks = map (mkCheck: mkCheck { inherit pkgs system configurations; }) mkHostChecks;
    touchHost = lib.findFirst (host:
      (systems.mkSystem host.module (lib.head systems.bootableTargets) systems.systemModuleFiles).config.settings.boot.touchKeyboard)
      null systemHosts;
    installerCheck = mkInstallerCheck {
      inherit pkgs system;
      prebuiltHost = (lib.head systemHosts).module;
      touchHost = if touchHost == null then null else touchHost.module;
    };
  in lib.listToAttrs (diskChecks ++ hostChecks ++ [ installerCheck (mkHostLayoutCheck { inherit pkgs; }) ]))
// lib.genAttrs (lib.unique (map (host: host.system) hosts.darwin)) (system:
  lib.listToAttrs (map (host:
    let username = systems.darwinConfigurations.${host.name}.config.settings.users.admin.username;
    in {
      name = "${host.name}-home";
      value = homes."${username}@${host.name}".activationPackage;
    }) (lib.filter (host: host.system == system) hosts.darwin)))
