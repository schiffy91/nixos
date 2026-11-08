{ flake ? builtins.getFlake "git+file://${toString ../../..}"
, inputs ? flake.inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  inherit (inputs.btrc.packages.${system}) btrcpy;
  cli = import ../../../config/pkgs/utils/nixosctl/package.nix { inherit pkgs btrcpy; lib = pkgs.lib; };
  common = { ... }: {
    # Exercise the newly built CLI by store path on an older host profile.
    # Calling /run/current-system's helper would deliberately fail this test.
    environment.systemPackages = [ pkgs.sbctl pkgs.openssl pkgs.gnutar
      (pkgs.writeShellScriptBin "nixosctl-secure-boot-keys" "echo wrong-host-helper >&2; exit 97")
    ];
    environment.etc."fixture-cli-path".text = "${cli}/bin/nixosctl";
    environment.etc."fixture-sbctl.conf".text = ''
      keydir: /var/lib/secureboot-fixture/sbctl/keys
      guid: /var/lib/secureboot-fixture/sbctl/GUID
    '';
    users.users.observer.isNormalUser = true;
    services.openssh = {
      enable = true;
      settings = { PermitRootLogin = "prohibit-password"; PasswordAuthentication = false; KbdInteractiveAuthentication = false; };
    };
    virtualisation.emptyDiskImages = [ 256 ];
    virtualisation.memorySize = 1024;
    virtualisation.cores = 1;
    systemd.services.prepare-key-fixture = {
      wantedBy = [ "multi-user.target" ];
      after = [ "dev-vdb.device" ];
      requires = [ "dev-vdb.device" ];
      path = [ pkgs.cryptsetup pkgs.e2fsprogs pkgs.coreutils pkgs.util-linux ];
      serviceConfig.Type = "oneshot";
      script = ''
        printf fixture-only | cryptsetup luksFormat --batch-mode --pbkdf pbkdf2 --iter-time 1 /dev/vdb -
        printf fixture-only | cryptsetup open --key-file - /dev/vdb fixture
        mkfs.ext4 -q /dev/mapper/fixture
        install -d -m0700 /var/lib/secureboot-fixture
        mount /dev/mapper/fixture /var/lib/secureboot-fixture
        chmod 0700 /var/lib/secureboot-fixture
      '';
    };
  };
in (import ../../vm/test.nix { inherit pkgs inputs; }) {
  name = "secure-boot-backup";
  nodes = { deck = common; fractal = common; };
  scenario = ./Main.btrc;
}
