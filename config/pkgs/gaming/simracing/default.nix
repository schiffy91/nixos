{ config, inputs, lib, pkgs, ... }:
let
  enabled = config.settings.apps.gaming.simracing.enable;
in {
  imports = [ inputs.logitech-trueforce.nixosModules.default ];

  # The upstream flake copies an unsigned .ko directly. Use the same Kbuild
  # modules_install path as other external drivers: strip, kmodigest-sign,
  # preserve the certificate, then compress. Keep upstream's source and module
  # configuration; remove this adjustment when its packaging does the same.
  options.boot.extraModulePackages = lib.mkOption {
    apply = packages: if !enabled then packages else map (package:
      if (package.pname or "") != "logitech-trueforce-driver" then package else
      package.overrideAttrs (_: {
        makeFlags = config.boot.kernelPackages.kernelModuleMakeFlags;
        buildPhase = ''
          runHook preBuild
          make -C ${config.boot.kernelPackages.kernel.dev}/lib/modules/${config.boot.kernelPackages.kernel.modDirVersion}/build \
            M="$PWD/mainline" ${lib.escapeShellArgs config.boot.kernelPackages.kernelModuleMakeFlags} modules
          runHook postBuild
        '';
        installPhase = ''
          runHook preInstall
          make -C ${config.boot.kernelPackages.kernel.dev}/lib/modules/${config.boot.kernelPackages.kernel.modDirVersion}/build \
            M="$PWD/mainline" ${lib.escapeShellArgs config.boot.kernelPackages.kernelModuleMakeFlags} \
            INSTALL_MOD_PATH="$out" INSTALL_MOD_DIR=extra modules_install
          runHook postInstall
        '';
      })
    ) packages;
  };

  config.hardware.logitech-trueforce.enable = lib.mkIf enabled true;
  config.settings.disk.immutability.persist.paths = lib.mkIf enabled [
    "${config.settings.users.admin.homeDirectory}/.config/logi-wheel"
    "${config.settings.users.admin.homeDirectory}/.local/share/logitech-trueforce"
  ];
}
