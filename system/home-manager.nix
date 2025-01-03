{ inputs, config, pkgs, lib, ...}: {
  imports = [ inputs.home-manager.nixosModules.home-manager ];
  home-manager = {
    extraSpecialArgs = { inherit inputs config pkgs lib; };
    useGlobalPkgs = true;
    useUserPackages = true;
    users = let
      # Use absolute path resolution
      usersPath = ../../users;
      
      # List all files in users directory
      allFiles = let 
        files = lib.filesystem.listFilesRecursive usersPath;
      in 
        builtins.trace "All files found: ${toString files}" files;
      
      # Filter for .nix files
      nixFiles = let 
        filtered = lib.filter (path: lib.hasSuffix ".nix" path) allFiles;
      in 
        builtins.trace "Nix files found: ${toString filtered}" filtered;
      
      # Map files to user configurations
      userAttrs = lib.map (path: 
        let 
          username = lib.removeSuffix ".nix" (baseNameOf path);
          imported = 
            if builtins.pathExists path
            then builtins.trace "Importing config for ${username}" (import path { inherit inputs config pkgs lib; })
            else builtins.throw "Config file not found: ${toString path}";
        in {
          name = username;
          value = imported;
        }
      ) nixFiles;
    in
      lib.listToAttrs userAttrs;
  };
}