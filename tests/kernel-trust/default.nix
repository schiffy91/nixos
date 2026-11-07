{ flake ? builtins.getFlake "git+file://${toString ../..}"
, inputs ? flake.inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  kernelPackages = pkgs.linuxPackages_latest;
  kernel = kernelPackages.kernel;
  probe = name: pkgs.stdenv.mkDerivation {
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
  };
  trusted = probe "trusted_probe";
  foreign = probe "foreign_probe";
in pkgs.testers.runNixOSTest {
  name = "kernel-trust";
  nodes.machine = { config, lib, ... }: {
    imports = [ ../../nix/settings.nix ../../nix/system/nixos/boot/kernel-trust.nix inputs.lanzaboote.nixosModules.lanzaboote ];
    boot.kernelPackages = kernelPackages;
    boot.extraModulePackages = [ trusted ];
    settings.boot.verifyModules = true;
    settings.sleep.allowHibernation = false;
    settings.sleep.hibernateOnSuspend = false;
    boot.loader.grub.enable = lib.mkForce false;
    boot.loader.systemd-boot.enable = true;
    boot.loader.efi.canTouchEfiVariables = true;
    boot.loader.timeout = 0;
    environment.systemPackages = [ pkgs.sbctl pkgs.sbsigntool pkgs.openssl pkgs.python3 pkgs.keyutils pkgs.kmodigest pkgs.zstd ];
    environment.etc."check-module-trust.py".source = ../../nix/system/nixos/boot/kernel-trust/check-modules.py;
    environment.etc."module-adversary.py".source = ./adversary.py;
    environment.etc."fixture-paths".text = builtins.toJSON {
      inherit trusted foreign;
      image = toString config.system.build.kernel;
      kernelFile = config.system.boot.loader.kernelFile;
    };
    environment.etc."test-sbctl.conf".text = ''
      keydir: /var/lib/kernel-trust-fixture/keys
      guid: /var/lib/kernel-trust-fixture/GUID
    '';
    environment.etc."generation-marker".text = lib.mkDefault "signature-only";
    specialisation.lockdown.configuration = {
      settings.boot.lockdown = "integrity";
      boot.loader.systemd-boot.enable = lib.mkForce false;
      boot.lanzaboote = {
        enable = true;
        pkiBundle = "/var/lib/kernel-trust-fixture";
        autoEnrollKeys = {
          enable = true;
          includeMicrosoftKeys = false;
          allowBrickingMyMachine = true;
        };
      };
      environment.etc."generation-marker".text = "lockdown";
    };
    virtualisation.useBootLoader = true;
    virtualisation.useEFIBoot = true;
    virtualisation.memorySize = 2048;
    virtualisation.cores = 2;
  };
  testScript = builtins.readFile ./test.py;
}
