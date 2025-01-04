{ inputs, config, lib, ... }: {
  imports = [ inputs.home-manager.nixosModules.home-manager ];
  home-manager = {
    useGlobalPkgs = true;
    useUserPackages = true;
    sharedModules = [ inputs.plasma-manager.homeManagerModules.plasma-manager ];
    extraSpecialArgs = { variables = config.variables; };
        users = 
          { "${config.variables.user.admin}" = import ../user/home.nix; } # 4) home.nix is the admin as defined in variables.nix
          // builtins.listToAttrs (map (path: { name = lib.removeSuffix ".nix" (baseNameOf path); value = import path; }) # 3) The name of the file is the name of the user; the content is for home manager. 
            (lib.filter (path: (lib.hasSuffix ".nix" path) && (baseNameOf path != "home.nix")) # 2) Filter by .nix – but ignore home.nix (that's special)
              (lib.filesystem.listFilesRecursive ../user))); # 1) Check every file in ../user
  };
}