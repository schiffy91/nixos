{ flake ? builtins.getFlake "git+file://${toString ../../..}"
, inputs ? flake.inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  inherit (inputs.btrc.packages.${system}) btrcpy;
  cli = import ../../../config/pkgs/utils/nixosctl/package.nix { inherit pkgs btrcpy; lib = pkgs.lib; };
  pki = "/var/lib/secure-boot-test";
  secure = { lib, ... }: {
    boot.loader.systemd-boot.enable = lib.mkForce false;
    boot.lanzaboote = {
      enable = true;
      pkiBundle = pki;
      configurationLimit = 10;
      settings.default = "nixos-generation-*.efi";
      autoEnrollKeys = {
        enable = false;
        includeMicrosoftKeys = false;
        # Upstream requires this even while auto-enrollment is disabled.
        allowBrickingMyMachine = true;
      };
    };
  };
  maintenance = { lib, ... }: {
    imports = [ secure ];
    boot.lanzaboote.autoEnrollKeys.enable = false;
    # Keep current, recovery, and the booted generation while they differ.
    boot.lanzaboote.configurationLimit = lib.mkForce 3;
    boot.lanzaboote.settings.secure-boot-enroll = "off";
  };
in (import ../../vm/test.nix { inherit pkgs inputs; }) {
  name = "secure-boot-removal";
  nodes.machine = { lib, ... }: {
    imports = [ inputs.lanzaboote.nixosModules.lanzaboote ];
    boot.loader.grub.enable = lib.mkForce false;
    boot.loader.systemd-boot.enable = true;
    boot.loader.efi.canTouchEfiVariables = true;
    boot.loader.timeout = 0;
    environment.systemPackages = [ cli pkgs.sbctl pkgs.sbsigntool pkgs.openssl pkgs.nixos-rebuild pkgs.git pkgs.binutils ];
    nix.settings.experimental-features = [ "nix-command" "flakes" ];
    environment.etc."fixture-build-tools.json".text = builtins.toJSON {
      rebuild = toString pkgs.nixos-rebuild;
    };
    environment.etc."sbctl/sbctl.conf".source = lib.mkDefault (pkgs.writeText "fixture-sbctl.conf" ''
      keydir: ${pki}/keys
      guid: ${pki}/GUID
    '');
    environment.etc."generation-marker".text = lib.mkDefault "setup";
    specialisation = {
      stale.configuration = {
        imports = [ secure ];
        boot.lanzaboote.autoEnrollKeys.enable = lib.mkForce true;
        boot.lanzaboote.autoEnrollKeys.allowBrickingMyMachine = lib.mkForce true;
        environment.etc."generation-marker".text = "stale";
      };
      maintenance.configuration = {
        imports = [ maintenance ];
        environment.etc."generation-marker".text = "maintenance";
      };
      recovery.configuration = {
        imports = [ maintenance ];
        environment.etc."generation-marker".text = "recovery";
      };
    };
    virtualisation.useBootLoader = true;
    virtualisation.useEFIBoot = true;
    virtualisation.tpm.enable = true;
    # This fixture boots disks, not PXE. Remove the unused NIC Option ROM so
    # own-key enrollment passes sbctl's real measured-boot safety checks.
    virtualisation.qemu.options = [ "-global virtio-net-pci.romfile=" ];
    virtualisation.memorySize = 1536;
    virtualisation.cores = 2;
  };
  scenario = ./Main.btrc;
}
