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
        let username = lib.removeSuffix ".nix" (baseNameOf path);
        in {
          name = builtins.trace "Processing user: ${username} from path: ${toString path}" username;
          value = import path;
        }
      ) nixFiles;
    in
      builtins.trace "Final user attributes: ${toString (map (x: x.name) userAttrs)}"
      (lib.listToAttrs userAttrs);
  };
}