{ inputs, lib, ...}: {
  imports = [inputs.home-manager.nixosModules.home-manager];
  home-manager = {
    extraSpecialArgs = { inherit inputs ; };
    useGlobalPkgs = true;
    useUserPackages = true;
    sharedModules = [ inputs.plasma-manager.homeManagerModules.plasma-manager ];
    users = lib.listToAttrs (lib.map (path: { 
      name = lib.removeSuffix ".nix" (baseNameOf path); 
      value = import path; 
    }) (lib.filter (path: lib.hasSuffix ".nix" path) (lib.filesystem.listFilesRecursive ../users)));
  };
}