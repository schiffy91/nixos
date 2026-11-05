{ lib, inputs, self, hostFiles, keyHostFiles, hostSystem, hostOutputName, mkSystem, systemModuleFiles, bootableTargets, flakeSources }:
let
  mkInstaller = system: prebuiltHostFiles:
    let hostFile = lib.findFirst (candidate: hostSystem candidate == system) (lib.head keyHostFiles) keyHostFiles;
        host = mkSystem hostFile (lib.head bootableTargets) systemModuleFiles;
    in import ./installer.nix {
      inherit inputs self system;
      authorizedKeys = host.config.settings.users.admin.authorizedKeys;
      stateVersion = host.config.system.stateVersion;
      prebuiltSystems = lib.listToAttrs (lib.concatMap (prebuilt:
        map (target:
          let built = (mkSystem prebuilt target systemModuleFiles).config.system.build;
          in {
            name = "${hostOutputName prebuilt}-${target}";
            value = { system = built.toplevel; disko = built.diskoScript; mount = built.mountScript; };
          }) bootableTargets) prebuiltHostFiles);
      prebuiltSources = lib.optionals (prebuiltHostFiles != [ ]) (flakeSources 3 inputs);
      prebuiltName = lib.concatMapStringsSep "-" hostOutputName prebuiltHostFiles;
      touchKeyboard = lib.any (prebuilt:
        (mkSystem prebuilt (lib.head bootableTargets) systemModuleFiles).config.boot.initrd.unl0kr.enable or false)
        prebuiltHostFiles;
    };
in {
  inherit mkInstaller;
  installerPackages = system:
    { installer = (mkInstaller system [ ]).config.system.build.isoImage; }
    // lib.listToAttrs (map (hostFile: {
         name = "installer-${hostOutputName hostFile}";
         value = (mkInstaller system [ hostFile ]).config.system.build.isoImage;
       }) (lib.filter (candidate: hostSystem candidate == system) hostFiles));
}
