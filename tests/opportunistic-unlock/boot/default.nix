{ system ? builtins.currentSystem }:
let
  locked = (builtins.fromJSON (builtins.readFile ../../../flake.lock)).nodes.nixpkgs.locked;
  pkgs = import (builtins.fetchTree { inherit (locked) type owner repo rev narHash; }) { inherit system; };
  inputs.btrc = let node = (builtins.fromJSON (builtins.readFile ../../../flake.lock)).nodes.btrc.locked;
    in builtins.getFlake "github:${node.owner}/${node.repo}/${node.rev}";
  worker = import ../../../config/system/nixos/opportunistic-unlock/package.nix { inherit pkgs inputs; };
in (import ../../vm/test.nix { inherit pkgs inputs; }) {
  name = "opportunistic-unlock";
  nodes.machine = { config, ... }: {
    _module.args.inputs = inputs;
    imports = [ ../../../config/settings.nix ../../../config/system/nixos/opportunistic-unlock.nix ];
    settings.disk.encryption.opportunisticUnlock.enable = true;
    settings.disk.label.root = "testroot";
    settings.users.admin.username = "volumeuser";
    virtualisation.useDefaultFilesystems = false;
    virtualisation.rootDevice = "/dev/mapper/testroot";
    virtualisation.fileSystems."/" = { device = "/dev/mapper/testroot"; fsType = "ext4"; };
    boot.initrd.luks.devices.testroot = {
      device = "/dev/vda";
      crypttabExtraOpts = [ "tpm2-device=auto" ];
    };
    boot.initrd.systemd.services.prepare-test-root = {
      wantedBy = [ "initrd.target" ];
      before = [ "systemd-cryptsetup@testroot.service" ];
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
    virtualisation.emptyDiskImages = [ 256 256 ];
    virtualisation.qemu.options = [
      "-device sdhci-pci,id=sdhci"
      "-blockdev driver=file,node-name=sdfile,filename=sdcard.raw"
      "-blockdev driver=raw,node-name=sdcard,file=sdfile"
      "-device sd-card,drive=sdcard,bus=sd-bus,id=sdcard-device"
      "-device qemu-xhci,id=xhci"
      "-blockdev driver=file,node-name=usbfile,filename=usb.raw"
      "-blockdev driver=raw,node-name=usbstore,file=usbfile"
      "-device usb-storage,drive=usbstore,id=usb-device,bus=xhci.0,serial=opportunistic-usb"
      "-drive file=nvme.raw,if=none,id=nvmestore,format=raw"
      "-device nvme,drive=nvmestore,serial=opportunistic-nvme"
    ];
    virtualisation.memorySize = 4096;
    virtualisation.cores = 2;
    virtualisation.tpm.enable = true;
    users.users.volumeuser = { isNormalUser = true; uid = 1234; };
    environment.systemPackages = [ worker pkgs.cryptsetup pkgs.keyutils pkgs.parted pkgs.procps ];
    environment.etc."test-cryptsetup".text = "${config.boot.initrd.systemd.package}/bin/systemd-cryptsetup";
    # Keep the production WantedBy; hold only the first boot so its key can be
    # inspected before consumption. The final reboot exercises automatic startup.
    systemd.services.nixos-opportunistic-unlock.unitConfig.ConditionPathExists = "/var/lib/test-auto-unlock";
    boot.kernelModules = [ "sdhci_pci" "mmc_block" "usb_storage" "xhci_pci" "sd_mod" "nvme" ];
  };
  scenario = ./Main.btrc;
}
