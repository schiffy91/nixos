{ inputs, config, lib, unstable-pkgs, ... }: {
  imports = [ inputs.home-manager.nixosModules.home-manager ];
  home-manager = {
    useGlobalPkgs = true;
    useUserPackages = true;
    backupFileExtension = "hm-backup";
    sharedModules = [ inputs.plasma-manager.homeManagerModules.plasma-manager ];
    extraSpecialArgs = { settings = config.settings; unstable-pkgs = unstable-pkgs; };
    users = { 
      "${config.settings.user.admin.username}" = { settings, ... }: {
        home = {
          username = settings.user.admin.username;
          homeDirectory = "/home/${settings.user.admin.username}";
          stateVersion = "24.11";
        };
        programs.home-manager.enable = true;
        imports = lib.filter (path: lib.hasSuffix ".nix" path) (lib.filesystem.listFilesRecursive ../apps);
      };
    };
  };
}