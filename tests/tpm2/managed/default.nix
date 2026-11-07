{ flake ? builtins.getFlake "git+file://${toString ../../..}"
, inputs ? flake.inputs
, system ? builtins.currentSystem
, reset ? false
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  nixosctl = import ../../../nix/apps/utils/nixosctl/package.nix {
    inherit pkgs;
    inherit (pkgs) lib;
    btrcpy = inputs.btrc.packages.${system}.btrcpy;
  };
  recovery = flake.nixosConfigurations.FRACTAL-NORTH-Secure-Boot.config.system.build.recoverySystem.config;
  recoveryUki = "${recovery.system.build.uki}/${recovery.system.boot.loader.ukiFile}";
  secureRoot = { lib, ... }: {
    imports = lib.optionals reset [ ../../../nix/system/nixos/immutability.nix ];
    settings.tpm.managedPolicy.enable = true;
    settings.disk.immutability = lib.mkIf reset {
      enable = true;
      mode = "reset";
      persist.paths = lib.mkForce [ "/etc/nixos" "/etc/machine-id" ];
    };
    settings.disk.subvolumes.volumes = lib.mkIf reset [
      { name = "@root"; mountPoint = "/"; flag = "root"; resetOnBoot = true; }
      { name = "@var"; mountPoint = "/var"; }
      { name = "@nix"; mountPoint = "/nix"; }
      { name = "@snapshots"; mountPoint = "/.snapshots"; flag = "snapshots"; }
    ];
    boot.initrd.luks.devices.root = {
      device = "/dev/vdb";
    };
    fileSystems = {
      "/" = {
        device = lib.mkForce "/dev/mapper/root";
        fsType = lib.mkIf reset (lib.mkForce "btrfs");
        options = lib.mkIf reset [ "subvol=@root" ];
      };
    } // lib.optionalAttrs reset (lib.genAttrs [ "/var" "/nix" "/.snapshots" ] (mount: {
      device = "/dev/mapper/root";
      fsType = "btrfs";
      options = [ "subvol=${{ "/var" = "@var"; "/nix" = "@nix"; "/.snapshots" = "@snapshots"; }.${mount}}" ];
    }));
  };
in pkgs.testers.runNixOSTest {
  name = "tpm-managed-policy";
  nodes.machine = { config, lib, pkgs, ... }: {
    imports = [
      inputs.lanzaboote.nixosModules.lanzaboote
      (inputs.lanzaboote + "/nix/tests/lanzaboote/common/lanzaboote.nix")
      ../../../nix/settings.nix
      ../../../nix/system/nixos/boot/tpm-policy.nix
    ];
    _module.args.inputs = inputs;
    lanzabooteTest.persistentRoot = true;
    boot.lanzaboote.configurationLimit = 3;
    boot.lanzaboote.autoEnrollKeys = {
      enable = true;
      includeMicrosoftKeys = false;
      allowBrickingMyMachine = true;
    };
    boot.initrd.systemd.tpm2.enable = true;
    boot.kernelParams = lib.mkAfter [ "console=ttyS0,115200n8" ];
    settings.disk.encryption.enable = true;
    settings.disk.by.partlabel.root = "/dev/vdb";
    settings.users.admin.username = "admin";
    users.users.admin = { isNormalUser = true; password = "testpass123"; };
    virtualisation = {
      tpm.enable = true;
      memorySize = 8192;
      cores = 4;
      emptyDiskImages = [ 2048 ];
    };
    image.repart.partitions.esp.repartConfig.SizeMinBytes = lib.mkForce "2G";
    services.getty.autologinUser = lib.mkForce null;
    environment.etc."tpm-test/recovery.efi".source = recoveryUki;
    environment.etc."tpm-test/recovery-system".text = toString recovery.system.build.toplevel;
    environment.etc."tpm-test/install-command".text = config.boot.lanzaboote.installCommand;
    environment.systemPackages = [ nixosctl pkgs.cryptsetup pkgs.python3 pkgs.jq pkgs.sbctl pkgs.sbsigntool pkgs.binutils pkgs.btrfs-progs pkgs.tpm2-tools pkgs.e2fsprogs ];
    environment.etc."nixos/config.json".text = builtins.toJSON {
      host_path = "/etc/nixos/nix/hosts/x86_64/QEMU.nix";
      target = "Secure-Boot";
    };
    environment.etc."nixos/nix/hosts/x86_64/QEMU.nix".text = "{}";
    # The actual CLI evaluates a bounded fixture of the node's actual settings.
    # All TPM, firmware, cryptsetup, and boot operations remain real.
    environment.etc."nixos/flake.nix".text = ''
      { outputs = { self }: {
        nixosConfigurations.QEMU-Secure-Boot.config.settings = {
          tpm.device = "/dev/tpmrm0";
          tpm.versionPath = "/sys/class/tpm/tpm0/tpm_version_major";
          tpm.managedPolicy.enable = ${lib.boolToString config.settings.tpm.managedPolicy.enable};
          disk.by.partlabel.root = "/dev/vdb";
        };
      }; }
    '';
    specialisation.secure-root.configuration = secureRoot;
    specialisation.updated.configuration = { ... }: {
      imports = [ secureRoot ];
      boot.kernelParams = [ "nixos.tpm.test=updated" ];
    };
  };
  testScript = { nodes, ... }:
    (import (inputs.lanzaboote + "/nix/tests/lanzaboote/common/image-helper.nix") {
      inherit (nodes) machine;
    }) + "reset = ${if reset then "True" else "False"}\n"
      + builtins.readFile ./common.py
      + builtins.readFile (if reset then ./reset.py else ./test.py);
}
