{ system ? builtins.currentSystem }:
let
  locked = (builtins.fromJSON (builtins.readFile ../../../flake.lock)).nodes.nixpkgs.locked;
  pkgs = import (builtins.fetchTree { inherit (locked) type owner repo rev narHash; }) { inherit system; };
  worker = import ../../../nix/system/nixos/sd-unlock/package.nix { inherit pkgs; systemd = pkgs.systemd; };
in pkgs.testers.runNixOSTest {
  name = "sd-unlock";
  nodes.machine = { config, ... }: {
    imports = [ ../../../nix/settings.nix ../../../nix/system/nixos/sd-unlock.nix ];
    settings.disk.encryption.unlockSD = true;
    settings.disk.label.root = "sdtestroot";
    settings.users.admin.username = "carduser";
    virtualisation.useDefaultFilesystems = false;
    virtualisation.rootDevice = "/dev/mapper/sdtestroot";
    virtualisation.fileSystems."/" = { device = "/dev/mapper/sdtestroot"; fsType = "ext4"; };
    boot.initrd.luks.devices.sdtestroot = {
      device = "/dev/vda";
      crypttabExtraOpts = [ "tpm2-device=auto" ];
    };
    boot.initrd.systemd.services.prepare-test-root = {
      wantedBy = [ "initrd.target" ];
      before = [ "systemd-cryptsetup@sdtestroot.service" ];
      after = [ "dev-vda.device" ];
      requires = [ "dev-vda.device" ];
      unitConfig.DefaultDependencies = false;
      serviceConfig.Type = "oneshot";
      path = [ pkgs.cryptsetup pkgs.e2fsprogs ];
      script = ''
        if ! cryptsetup isLuks /dev/vda; then
          printf testpass123 | cryptsetup luksFormat --batch-mode --pbkdf pbkdf2 --iter-time 1 /dev/vda -
          printf testpass123 | cryptsetup open --key-file - /dev/vda fixture-root
          mkfs.ext4 -q /dev/mapper/fixture-root
          cryptsetup close fixture-root
        fi
      '';
    };
    virtualisation.emptyDiskImages = [ 256 ];
    virtualisation.qemu.options = [
      "-device sdhci-pci,id=sdhci"
      "-blockdev driver=file,node-name=sdfile,filename=sdcard.raw"
      "-blockdev driver=raw,node-name=sdcard,file=sdfile"
      "-device sd-card,drive=sdcard,bus=sd-bus,id=sdcard-device"
    ];
    virtualisation.memorySize = 4096;
    virtualisation.cores = 2;
    virtualisation.tpm.enable = true;
    users.users.carduser = { isNormalUser = true; uid = 1234; };
    environment.systemPackages = [ worker pkgs.cryptsetup pkgs.keyutils pkgs.parted ];
    environment.etc."test-cryptsetup".text = "${config.boot.initrd.systemd.package}/bin/systemd-cryptsetup";
    # Keep the production WantedBy; hold only the first boot so its key can be
    # inspected before consumption. The final reboot exercises automatic startup.
    systemd.services.nixos-sd-unlock.unitConfig.ConditionPathExists = "/var/lib/test-auto-unlock";
    boot.kernelModules = [ "sdhci_pci" "mmc_block" ];
  };
  testScript = builtins.readFile ./test.py;
}
