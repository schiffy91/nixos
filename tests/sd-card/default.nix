{ inputs ? (builtins.getFlake "git+file://${toString ../..}").inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  programs = import ../../config/pkgs/utils/drive/package.nix { inherit pkgs inputs; };
  cli = import ../../config/pkgs/utils/nixosctl/package.nix { inherit pkgs; inherit (inputs.btrc.packages.${system}) btrcpy; lib = pkgs.lib; };
in (import ../vm/test.nix { inherit pkgs inputs; }) {
  name = "sd-card";
  nodes.machine = { ... }: {
    _module.args.inputs = inputs;
    imports = [ ../../config/settings.nix ../../config/system/nixos/sd-card.nix ];
    settings.disk.sdCard = { enable = true; library = "Library"; driveFolder = "Games/Library"; };
    settings.users.admin.username = "carduser";
    users.users.carduser = { isNormalUser = true; uid = 1000; };
    boot.kernelModules = [ "sdhci_pci" "mmc_block" "fuse" ];
    virtualisation.qemu.options = [
      "-device sdhci-pci,id=sdhci"
      "-blockdev driver=file,node-name=sdfile,filename=sdcard.raw"
      "-blockdev driver=raw,node-name=sdcard,file=sdfile"
      "-device sd-card,drive=sdcard,bus=sd-bus,id=sdcard-device"
    ];
    environment.systemPackages = [ cli programs.drive programs.fake pkgs.cryptsetup pkgs.curl pkgs.attr pkgs.python3 ];
    virtualisation.memorySize = 4096;  # opening the container derives its key in memory
    virtualisation.cores = 2;
  };
  scenario = ./Main.btrc;
  extraFiles = [ ../drive/Fixture.btrc ];
}
