{ inputs, config, pkgs, lib, ...}: {
  imports = [ inputs.home-manager.nixosModules.home-manager ];
  home-manager = {
    extraSpecialArgs = { inherit inputs config pkgs lib; };
    useGlobalPkgs = true;
    useUserPackages = true;
    users = let
      allFiles = builtins.trace "All files found: " (lib.filesystem.listFilesRecursive ../users);
      nixFiles = builtins.trace "Filtered .nix files: " (lib.filter (path: lib.hasSuffix ".nix" path) allFiles);
      userAttrs = lib.map (path: {
        name = builtins.trace "Processing user: " (lib.removeSuffix ".nix" (baseNameOf path));
        value = import path;
      }) nixFiles;
    in
      builtins.trace "Final user attributes: " (lib.listToAttrs userAttrs);
  };
}