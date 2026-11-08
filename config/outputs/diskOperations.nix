{ inputs, lib, hosts, systems }:
let
  targets = [ "Disk-Operation" ];
  mkDiskoSystem = hostFile: target:
    let
      host = systems.mkSystem hostFile (lib.head systems.bootableTargets) systems.systemModuleFiles;
    in lib.nixosSystem {
      system = hosts.hostSystem hostFile;
      specialArgs = { inherit inputs; };
      modules = [
        ../settings.nix
        ../system/nixos/disk.nix
        {
          settings.disk = host.config.settings.disk;
          settings.boot.method = target;
          nixpkgs.config.allowUnfree = true;
          system.stateVersion = "24.11";
        }
      ];
    };
in {
  inherit targets mkDiskoSystem;
  diskoConfigurations = lib.listToAttrs (lib.concatMap (host:
    map (target: {
      name = "${hosts.hostOutputName host.module}-${target}";
      value.disko.devices = (mkDiskoSystem host.module target).config.disko.devices;
    }) targets) hosts.linux);
}
