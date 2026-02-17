{ inputs, config, pkgs-unstable, lib, ... }: {
  ##### Security #####
  users.mutableUsers = false;
  users.users.${config.settings.user.admin.username} = {
    isNormalUser = true;
    extraGroups = [ "wheel" "libvirtd" "networkmanager" ];
    hashedPasswordFile = "${config.settings.secrets.path}/${config.settings.secrets.hashedPasswordFile}";
    openssh.authorizedKeys.keys = [
      "${config.settings.user.admin.authorizedKey} ${config.settings.user.admin.username}"
    ];
  };
  ##### Auto Login #####
  services.displayManager.autoLogin = {
    enable = config.settings.user.admin.autoLogin.enable;
    user = config.settings.user.admin.username;
  };
  ##### Home Manager #####
  imports = [ inputs.home-manager.nixosModules.home-manager ];
  home-manager = {
    useGlobalPkgs = true;
    useUserPackages = true;
    backupFileExtension = "hm-backup";
    sharedModules = [ inputs.plasma-manager.homeModules.plasma-manager ];
    extraSpecialArgs = { settings = config.settings; pkgs-unstable = pkgs-unstable; };
    users = { 
      "${config.settings.user.admin.username}" = { settings, ... }: {
        home = {
          username = settings.user.admin.username;
          homeDirectory = "/home/${settings.user.admin.username}";
          stateVersion = "24.11";
        };
        programs.home-manager.enable = config.settings.user.admin.homeManager.enable;
        imports = lib.filter (path: lib.hasSuffix ".nix" path) (lib.filesystem.listFilesRecursive ../apps);
      };
    };
  };
}