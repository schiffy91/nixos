# A minimal installer ISO that needs no typing: the admin's keys are already
# authorized, this repository is already at /etc/nixos, and nixosctl is already
# built, so a booted stick goes straight to `nixosctl install`.
{ inputs, self, system, authorizedKeys, stateVersion, prebuiltSystems ? { }, prebuiltSources ? [ ], prebuiltName ? "", touchKeyboard ? false }:
let
  pkgs = import inputs.nixpkgs { inherit system; config.allowUnfree = true; };
  lib = inputs.nixpkgs.lib;
  btrcpy = inputs.btrc.packages.${system}.btrcpy;
  nixosctl = import ../../../pkgs/utils/nixosctl/package.nix { inherit pkgs lib btrcpy; };

  installerSystem = lib.nixosSystem {
    inherit system;
    modules = [
      ./touch-console.nix
      "${inputs.nixpkgs}/nixos/modules/installer/cd-dvd/installation-cd-minimal.nix"
      (import ./environment.nix {
        inherit authorizedKeys stateVersion;
        safeGitDirectories = [ "/etc/nixos" "/mnt/etc/nixos" ];
      })
      {
        networking.hostName = "nixos-installer";
        image.baseName = lib.mkForce ("nixos-installer-${system}" + lib.optionalString (prebuiltName != "") "-${prebuiltName}");
        boot.loader.timeout = lib.mkForce 1;
        boot.kernelParams = [ "console=tty0" "console=ttyS0,115200n8" ];
        services.buffyboard.enable = touchKeyboard;  # a host that unlocks by touch installs by touch too
        isoImage = {
          forceTextMode = true;
          squashfsCompression = "zstd -Xcompression-level 15";
          makeEfiBootable = true;
          makeUsbBootable = true;
          storeContents = lib.concatMap (entry: [ entry.system entry.disko entry.mount ])
            (lib.attrValues prebuiltSystems) ++ prebuiltSources;
        };
        nixpkgs.config.allowUnfree = true;

        environment.etc."nixos-installer/sources.json".text = builtins.toJSON prebuiltSources;
        environment.etc."nixos-installer/prebuilt.json".text = builtins.toJSON
          (lib.mapAttrs (_: entry: {
            system = "${entry.system}";
            disko = "${entry.disko}";
            mount = "${entry.mount}";
          }) prebuiltSystems);
        environment.systemPackages = [ nixosctl pkgs.gnumake pkgs.gcc pkgs.ripgrep ];

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
