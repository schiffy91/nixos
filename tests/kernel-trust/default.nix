{ flake ? builtins.getFlake "git+file://${toString ../..}"
, inputs ? flake.inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  kernelPackages = pkgs.linuxPackages_latest;
  probe = name: import ./probe.nix { inherit pkgs kernelPackages name; };
  trusted = probe "trusted_probe";
  foreign = probe "foreign_probe";
in pkgs.testers.runNixOSTest {
  name = "kernel-trust";
  nodes.machine = { config, lib, ... }: {
    _module.args.inputs = inputs;
    imports = [ ../../config/settings.nix ../../config/system/nixos/boot/kernel-trust.nix inputs.lanzaboote.nixosModules.lanzaboote ];
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
    environment.etc."check-module-trust".source = "${import ../../config/system/nixos/boot/kernel-trust/package.nix { inherit pkgs inputs; }}/bin/check-module-trust";
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
