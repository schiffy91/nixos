{ inputs, self }:
let
  lib = inputs.nixpkgs.lib;
  hosts = import ./hosts.nix { inherit lib; };
  systems = import ./systems.nix { inherit inputs self lib hosts; };
  diskOperations = import ./diskOperations.nix { inherit inputs lib hosts systems; };
  installers = import ./installers.nix { inherit inputs self lib hosts systems; };
  homes = import ./homes.nix { inherit lib hosts systems; };
  publicHosts = map (host: {
    inherit (host) name system;
    outputName = hosts.hostOutputName host.module;
    path = lib.removePrefix "${toString ../hosts}/" (toString host.module);
  }) hosts.physical;
in {
  lib.hosts = publicHosts;
  inherit (systems) nixosConfigurations darwinConfigurations;
  inherit (diskOperations) diskoConfigurations;
  packages = import ./packages.nix { inherit inputs; installerPackages = installers.packages; };
  homeConfigurations = homes;
  checks = import ./checks.nix { inherit inputs lib hosts publicHosts systems diskOperations installers homes; };
}
