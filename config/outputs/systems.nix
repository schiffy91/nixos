{ inputs, self, lib, hosts }:
let
  bootableTargets = [ "Standard-Boot" "Secure-Boot" ];
  systemModuleFiles = [ ../system/nixos ../pkgs ../pkgs/de/kde ];
  # A system from any host module, registered or not: the VM tests write theirs at run time.
  systemWith = { system, target, module, modules ? systemModuleFiles }:
    let
      pkgs-unstable = import inputs.nixpkgs-unstable { inherit system; config.allowUnfree = true; };
    in lib.nixosSystem {
      inherit system;
      specialArgs = { inherit self inputs pkgs-unstable; };
      modules = [
        ../settings.nix
        module
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
  mkSystem = hostFile: target: modules:
    systemWith { system = hosts.hostSystem hostFile; module = hostFile; inherit target modules; };
in {
  inherit bootableTargets systemModuleFiles mkSystem systemWith;
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
