# A minimal installer ISO that needs no typing: the admin's keys are already
# authorized, this repository is already at /etc/nixos, and nixosctl is already
# built, so a booted stick goes straight to `nixosctl install`.
{ inputs, self, system, authorizedKeys, stateVersion }:
let
  pkgs = import inputs.nixpkgs { inherit system; config.allowUnfree = true; };
  lib = inputs.nixpkgs.lib;
  btrcpy = inputs.btrc.packages.${system}.btrcpy;
  nixosctl = import ../../../apps/utils/nixosctl/package.nix { inherit pkgs lib btrcpy; };

  installerSystem = lib.nixosSystem {
    inherit system;
    modules = [
      "${inputs.nixpkgs}/nixos/modules/installer/cd-dvd/installation-cd-minimal.nix"
      (import ./environment.nix {
        inherit authorizedKeys stateVersion;
        safeGitDirectories = [ "/etc/nixos" "/mnt/etc/nixos" ];
      })
      {
        networking.hostName = "nixos-installer";
        image.baseName = lib.mkForce "nixos-installer-${system}";
        isoImage = {
          squashfsCompression = "zstd -Xcompression-level 15";
          makeEfiBootable = true;
          makeUsbBootable = true;
        };
        nixpkgs.config.allowUnfree = true;
        environment.systemPackages = [ nixosctl pkgs.gnumake pkgs.gcc ];

        # /etc/nixos has to be writable: nixosctl install writes config.json into it.
        systemd.services.installer-repository = {
          description = "Place this repository at /etc/nixos";
          wantedBy = [ "multi-user.target" ];
          before = [ "getty.target" ];
          serviceConfig = {
            Type = "oneshot";
            RemainAfterExit = true;
          };
          script = ''
            ${pkgs.coreutils}/bin/mkdir -p /etc/nixos
            ${pkgs.rsync}/bin/rsync -a --chmod=u+w ${self}/ /etc/nixos/
          '';
        };
      }
    ];
  };
in installerSystem
