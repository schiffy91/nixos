{ inputs, config, pkgs, lib, ...}: {
  imports = [ inputs.home-manager.nixosModules.home-manager ];
  home-manager = {
    extraSpecialArgs = { inherit inputs config pkgs lib; };
    useGlobalPkgs = true;
    useUserPackages = true;
    users = let
      allFiles = let files = lib.filesystem.listFilesRecursive ../users; 
                 in builtins.trace "All files found: ${toString files}" files;
                 
      nixFiles = let filtered = lib.filter (path: lib.hasSuffix ".nix" path) allFiles;
                 in builtins.trace "Nix files found: ${toString filtered}" filtered;
                 
userAttrs = lib.map (path: 
  let 
    username = lib.removeSuffix ".nix" (baseNameOf path);
    imported = 
      if builtins.pathExists path
      then builtins.trace "Importing config for ${username}" (import path)
      else builtins.throw "Config file not found: ${toString path}";
  in {
    name = username;
    value = builtins.trace "Loaded config for ${username}: ${toString (builtins.attrNames imported)}" imported;
  }
) nixFiles;
    in
      builtins.trace "Final user attributes: ${toString (map (x: x.name) userAttrs)}"
      (lib.listToAttrs userAttrs);
  };
}