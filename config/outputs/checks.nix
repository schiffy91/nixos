{ inputs, lib, hosts, publicHosts, systems, diskOperations, installers, homes }:
let
  mkHostLayoutCheck = import ../../tests/checks/host_layout.nix {
    inherit lib hosts publicHosts systems diskOperations homes;
  };
  mkSshTrustCheck = import ../../tests/checks/ssh_trust.nix { inherit lib hosts systems; };
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
    ../../tests/checks/graphics_layouts.nix
    ../../tests/checks/app_hierarchy.nix
    ../../tests/checks/persistence_ownership.nix
  ];
  mkEvaluationCheck = import ../../tests/checks/evaluations.nix {
    inherit lib;
    outputs = { inherit (systems) nixosConfigurations; inherit (diskOperations) diskoConfigurations; };
  };
  expectations = file: builtins.fromJSON (builtins.readFile file);
  # Both architectures' QEMU hosts are evaluated wherever the check is built: evaluation needs no builder.
  qemuHostExpectations = lib.concatMap (architecture: map (expectation:
    lib.mapAttrs (_: lib.replaceStrings [ "{{arch}}" ] [ architecture ]) expectation)
    (expectations ../../tests/checks/qemu_host.json)) [ "x86_64" "aarch64" ];
  deckAssertions = lib.concatMap (target:
    let
      deck = systems.nixosConfigurations."STEAM-DECK-OLED-${target}".config;
      wanted = [ "amdgpu.lockup_timeout=5000,10000,10000,5000" "ttm.pages_min=2097152" "log_buf_len=4M" "audit=0" "spi_amd.speed_dev=1" "mem_sleep_default=deep" ];
    in [
      { holds = !(builtins.elem "amd_iommu=off" deck.boot.kernelParams); message = "STEAM-DECK-OLED-${target}: amd_iommu=off is set"; }
      { holds = builtins.all (parameter: builtins.elem parameter deck.boot.kernelParams) wanted; message = "STEAM-DECK-OLED-${target}: a Deck kernel parameter is missing"; }
      { holds = !(deck.systemd.services.systemd-suspend.serviceConfig ? ExecStart); message = "STEAM-DECK-OLED-${target}: systemd-suspend's ExecStart is replaced"; }
    ]) systems.bootableTargets;
  storeAssertions = lib.concatMap (host: map (target:
    let
      name = "${hosts.hostOutputName host.module}-${target}";
      nix = systems.nixosConfigurations.${name}.config.nix.settings;
    in { holds = (nix.min-free or 0) > 0 && nix.max-free > nix.min-free; message = "${name}: no free-space floor for the Nix store (settings.nix.minFreeGiB)"; })
    systems.bootableTargets) (lib.filter (host: host.name != "QEMU") hosts.physical);
  # Unit and contract suites that are one derivation each. They were graph nodes and lint built them
  # as well; as checks they are built once, with every other check.
  unitChecks = system: {
    opportunistic-unlock-policy = import ../../tests/opportunistic-unlock/unit { inherit system; };
    secure-boot-key-contracts = import ../../tests/secure-boot/unit.nix { inherit inputs system; };
    sleep-lock-state = import ../../tests/sleep-lock/unit { inherit inputs system; };
    networking-policy = import ../../tests/networking/policy { inherit system; };
    recovery-mount-contracts = import ../../tests/recovery/unit.nix { inherit inputs system; };
    recovery-artifacts = import ../../tests/recovery/artifacts { inherit system; };
  };
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
  in lib.listToAttrs (diskChecks ++ hostChecks ++ [ installerCheck (mkHostLayoutCheck { inherit pkgs; }) (mkSshTrustCheck { inherit pkgs; })
    {  # the tool is tested in its own repository; here its test holds this repository's job table against its workflow
      name = "ci-local";
      value = import "${inputs.ci-local}/tests/ci_local.nix" { inherit pkgs; table = ../../ci/local.json; workflow = ../../.github/workflows/checks.yml; jobs = [ "lint" "checks" ]; };
    }
    (mkEvaluationCheck { inherit pkgs; name = "app-settings"; expectations = expectations ../../tests/checks/app_settings.json; assertions = deckAssertions ++ storeAssertions; })
    (mkEvaluationCheck { inherit pkgs; name = "qemu-hosts"; expectations = qemuHostExpectations; }) ]) // unitChecks system)
// lib.genAttrs (lib.unique (map (host: host.system) hosts.darwin)) (system:
  lib.listToAttrs (map (host:
    let username = systems.darwinConfigurations.${host.name}.config.settings.users.admin.username;
    in {
      name = "${host.name}-home";
      value = homes."${username}@${host.name}".activationPackage;
    }) (lib.filter (host: host.system == system) hosts.darwin)))
