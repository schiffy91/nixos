{ inputs, self, lib, hosts, systems }:
let
  # The sources of the inputs, for a machine that is installed without a network and evaluates afterwards.
  # This repository is not one of them: the installer carries it as the checkout it installs (without
  # the files that change nothing in a system), and naming it here made a README edit a new image.
  flakeSources = depth: attrs: lib.unique (lib.concatMap (input:
    lib.optional (input ? outPath) "${input.outPath}"
    ++ lib.optionals (depth > 0 && input ? inputs) (flakeSources (depth - 1) input.inputs))
    (lib.attrValues (removeAttrs attrs [ "self" ])));
  mkInstaller = system: prebuiltHostFiles:
    let
      keyHost = lib.findFirst (candidate: candidate.system == system) (lib.head hosts.physical) hosts.physical;
      host = systems.systemOf keyHost.module (lib.head systems.bootableTargets);
    in import ../system/nixos/images/installer.nix {
      inherit inputs self system;
      authorizedKeys = host.config.settings.users.admin.authorizedKeys;
      stateVersion = host.config.system.stateVersion;
      prebuiltSystems = lib.listToAttrs (lib.concatMap (prebuilt:
        map (target:
          let built = (systems.systemOf prebuilt target).config.system.build;
          in {
            name = "${hosts.hostOutputName prebuilt}-${target}";
            value = { system = built.toplevel; disko = built.diskoScript; mount = built.mountScript; };
          }) systems.bootableTargets) prebuiltHostFiles);
      prebuiltSources = lib.optionals (prebuiltHostFiles != [ ]) (flakeSources 3 inputs);
      prebuiltName = lib.concatMapStringsSep "-" hosts.hostOutputName prebuiltHostFiles;
      touchKeyboard = lib.any (prebuilt:
        (systems.systemOf prebuilt (lib.head systems.bootableTargets)).config.settings.boot.touchKeyboard)
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
