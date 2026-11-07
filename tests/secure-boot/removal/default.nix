{ flake ? builtins.getFlake "git+file://${toString ../../..}"
, inputs ? flake.inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  pki = "/var/lib/secure-boot-test";
  secure = { lib, ... }: {
    boot.loader.systemd-boot.enable = lib.mkForce false;
    boot.lanzaboote = {
      enable = true;
      pkiBundle = pki;
      configurationLimit = 10;
      autoEnrollKeys = {
        enable = lib.mkDefault true;
        includeMicrosoftKeys = false;
        allowBrickingMyMachine = true;
      };
    };
  };
  maintenance = { lib, ... }: {
    imports = [ secure ];
    boot.lanzaboote.autoEnrollKeys.enable = false;
    boot.lanzaboote.configurationLimit = lib.mkForce 2;
    boot.lanzaboote.settings.secure-boot-enroll = "off";
  };
in pkgs.testers.runNixOSTest {
  name = "secure-boot-removal";
  nodes.machine = { lib, ... }: {
    imports = [ inputs.lanzaboote.nixosModules.lanzaboote ];
    boot.loader.grub.enable = lib.mkForce false;
    boot.loader.systemd-boot.enable = true;
    boot.loader.efi.canTouchEfiVariables = true;
    boot.loader.timeout = 0;
    environment.systemPackages = [ pkgs.sbctl pkgs.sbsigntool pkgs.openssl ];
    environment.etc."test-sbctl.conf".text = ''
      keydir: ${pki}/keys
      guid: ${pki}/GUID
    '';
    environment.etc."generation-marker".text = lib.mkDefault "setup";
    specialisation = {
      enrolled.configuration = {
        imports = [ secure ];
        environment.etc."generation-marker".text = "enrolled";
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
    virtualisation.memorySize = 1536;
    virtualisation.cores = 2;
  };
  testScript = builtins.readFile ./test.py;
}
