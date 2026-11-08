{ flake ? builtins.getFlake "git+file://${toString ../../..}"
, inputs ? flake.inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
in (import ../../vm/test.nix { inherit pkgs inputs; }) {
  name = "boot-fallback";
  nodes.machine = { lib, ... }: {
    boot.loader.grub.enable = lib.mkForce false;
    boot.loader.systemd-boot = { enable = true; configurationLimit = 1; };
    boot.loader.efi.canTouchEfiVariables = true;
    environment.etc."generation-marker".text = lib.mkDefault "baseline";
    specialisation.updated.configuration.environment.etc."generation-marker".text = "updated";
    virtualisation.useBootLoader = true;
    virtualisation.useEFIBoot = true;
    virtualisation.memorySize = 1024;
  };
  scenario = ./Main.btrc;
}
