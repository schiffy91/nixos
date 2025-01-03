{ inputs, config, pkgs, lib, ...}: {
  imports = [ inputs.home-manager.nixosModules.home-manager ];
  home-manager = {
    extraSpecialArgs = { inherit inputs config pkgs lib; };
    useGlobalPkgs = true;
    useUserPackages = true;
    users = lib.listToAttrs (lib.map (path: { 
      name = lib.removeSuffix ".nix" (baseNameOf path); 
      value = import path; 
    }) (lib.filter (path: lib.hasSuffix ".nix" path) (lib.filesystem.listFilesRecursive ../users)));
  };
}