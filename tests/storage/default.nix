{ inputs ? (builtins.getFlake "git+file://${toString ../..}").inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  programs = inputs.drive.packages.${system};
  cli = import ../../config/pkgs/utils/nixosctl/package.nix { inherit pkgs inputs; inherit (inputs.btrc.packages.${system}) btrcpy; lib = pkgs.lib; };
in (import ../lib/vm-test.nix { inherit pkgs inputs; }) {
  name = "storage";
  nodes.machine = { ... }: {
    _module.args.inputs = inputs;
    imports = [ ../../config/settings.nix ../../config/system/nixos/storage.nix ];
    # Two volumes of two kinds: a card that may hold a library and is checked against the Drive, and a plain USB drive.
    settings.disk.storage.sdcard = { label = "SD"; mountPoint = "/sd"; library = "Library"; driveFolder = "Games/Library"; };
    settings.disk.storage.usb = { label = "USB"; };
    settings.users.admin.username = "carduser";
    users.users.carduser = { isNormalUser = true; uid = 1000; };
    boot.kernelModules = [ "sdhci_pci" "mmc_block" "fuse" "xhci_pci" "usb_storage" "sd_mod" ];
    virtualisation.qemu.options = [
      "-device sdhci-pci,id=sdhci"
      "-blockdev driver=file,node-name=sdfile,filename=sdcard.raw"
      "-blockdev driver=raw,node-name=sdcard,file=sdfile"
      "-device sd-card,drive=sdcard,bus=sd-bus,id=sdcard-device"
      "-device qemu-xhci,id=storage-usb"
      "-drive if=none,id=usbdrive,file=usbdrive.raw,format=raw"
      "-device usb-storage,bus=storage-usb.0,drive=usbdrive,serial=STORAGE"
    ];
    environment.systemPackages = [ cli programs.drive programs.fake pkgs.cryptsetup pkgs.curl pkgs.attr pkgs.python3 ];
    virtualisation.memorySize = 4096;  # opening the container derives its key in memory
    # The machine must be able to sleep: a store shared with the host cannot be frozen, so it gets an image of its own.
    virtualisation.useNixStoreImage = true;
    virtualisation.writableStore = true;
    virtualisation.sharedDirectories = pkgs.lib.mkForce { };
    systemd.sleep.settings.Sleep.MemorySleepMode = "s2idle";
    services.logind.settings.Login.HandlePowerKey = "ignore";  # the power button is how the test wakes it
    boot.kernelParams = [ "no_console_suspend" ];
    virtualisation.cores = 2;
  };
  scenario = ./Main.btrc;
  extraFiles = [ ./DriveFixture.btrc ];  # the helpers of schiffy91/drive's own tests, as they were when the client moved: this scenario runs the client beside a card
}
