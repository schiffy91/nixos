{ inputs, self, lib, hosts, systems }:
let
  flakeSources = depth: attrs: lib.unique (lib.concatMap (input:
    lib.optional (input ? outPath) "${input.outPath}"
    ++ lib.optionals (depth > 0 && input ? inputs) (flakeSources (depth - 1) input.inputs))
    (lib.attrValues attrs));
  mkInstaller = system: prebuiltHostFiles:
    let
      keyHost = lib.findFirst (candidate: candidate.system == system) (lib.head hosts.physical) hosts.physical;
      host = systems.mkSystem keyHost.module (lib.head systems.bootableTargets) systems.systemModuleFiles;
    in import ../system/nixos/images/installer.nix {
      inherit inputs self system;
      authorizedKeys = host.config.settings.users.admin.authorizedKeys;
      stateVersion = host.config.system.stateVersion;
      prebuiltSystems = lib.listToAttrs (lib.concatMap (prebuilt:
        map (target:
          let built = (systems.mkSystem prebuilt target systems.systemModuleFiles).config.system.build;
          in {
            name = "${hosts.hostOutputName prebuilt}-${target}";
            value = { system = built.toplevel; disko = built.diskoScript; mount = built.mountScript; };
          }) systems.bootableTargets) prebuiltHostFiles);
      prebuiltSources = lib.optionals (prebuiltHostFiles != [ ]) (flakeSources 3 inputs);
      prebuiltName = lib.concatMapStringsSep "-" hosts.hostOutputName prebuiltHostFiles;
      touchKeyboard = lib.any (prebuilt:
        (systems.mkSystem prebuilt (lib.head systems.bootableTargets) systems.systemModuleFiles).config.settings.boot.touchKeyboard)
        prebuiltHostFiles;
    };
  installerPackages = system:
    { installer = (mkInstaller system [ ]).config.system.build.isoImage; }
    // lib.listToAttrs (map (host: {
      name = "installer-${hosts.hostOutputName host.module}";
      value = (mkInstaller system [ host.module ]).config.system.build.isoImage;
    }) (lib.filter (host: host.system == system) hosts.linux));
in {
  inherit mkInstaller flakeSources;
  packages = lib.genAttrs hosts.systems installerPackages;
}
