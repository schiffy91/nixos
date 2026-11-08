{ pkgs, kernelPackages ? pkgs.linuxPackages_latest, name ? "native_probe" }:
let kernel = kernelPackages.kernel;
in pkgs.stdenv.mkDerivation {
    pname = name;
    version = "1";
    src = pkgs.runCommand "${name}-source" { } ''
      mkdir -p $out
      cp ${./probe.c} $out/${name}.c
      echo 'obj-m := ${name}.o' > $out/Makefile
    '';
    nativeBuildInputs = kernel.moduleBuildDependencies;
    buildPhase = ''
      make -C ${kernel.dev}/lib/modules/${kernel.modDirVersion}/build M="$PWD" \
        ${pkgs.lib.escapeShellArgs kernelPackages.kernelModuleMakeFlags} modules
    '';
    installPhase = ''
      make -C ${kernel.dev}/lib/modules/${kernel.modDirVersion}/build M="$PWD" \
        ${pkgs.lib.escapeShellArgs kernelPackages.kernelModuleMakeFlags} \
        INSTALL_MOD_PATH="$out" INSTALL_MOD_DIR=extra modules_install
    '';
  }
