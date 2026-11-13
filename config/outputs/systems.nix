{ inputs, self, lib, hosts }:
let
  bootableTargets = [ "Standard-Boot" "Secure-Boot" ];
  systemModuleFiles = [ ../system/nixos ../pkgs ../pkgs/de/kde ];
  mkSystem = hostFile: target: modules:
    let
      system = hosts.hostSystem hostFile;
      pkgs-unstable = import inputs.nixpkgs-unstable { inherit system; config.allowUnfree = true; };
    in lib.nixosSystem {
      inherit system;
      specialArgs = { inherit self inputs pkgs-unstable; };
      modules = [
        ../settings.nix
        hostFile
        {
          settings.boot.method = lib.mkForce target;
          nix = {
            channel.enable = false;
            settings.experimental-features = [ "nix-command" "flakes" ];
          };
          nixpkgs.config.allowUnfree = true;
          system.stateVersion = "24.11";
        }
      ] ++ modules;
    };
in {
  inherit bootableTargets systemModuleFiles mkSystem;
  nixosConfigurations = lib.listToAttrs (lib.concatMap (host:
    map (target: {
      name = "${hosts.hostOutputName host.module}-${target}";
      value = mkSystem host.module target systemModuleFiles;
    }) bootableTargets) hosts.linux);
  darwinConfigurations = lib.listToAttrs (map (host: {
    name = host.name;
    value = inputs.nix-darwin.lib.darwinSystem {
      inherit (host) system;
      specialArgs = { inherit inputs; };
      modules = [ ../settings.nix ../system/darwin ../pkgs/darwin.nix host.module ];
    };
  }) hosts.darwin);
}
