{ inputs, config, unstable-pkgs, lib, ... }: {
  ##### Home Manager #####
  imports = [ inputs.home-manager.nixosModules.home-manager ];
  home-manager = {
    useGlobalPkgs = true;
    useUserPackages = true;
    backupFileExtension = "hm-backup";
    sharedModules = if (lib.hasInfix "plasma" config.settings.desktop.environment) then [ inputs.plasma-manager.homeManagerModules.plasma-manager ] else [];
    extraSpecialArgs = { settings = config.settings; unstable-pkgs = unstable-pkgs; };
    users = { 
      "${config.settings.user.admin.username}" = { settings, ... }: {
        home = {
          username = settings.user.admin.username;
          homeDirectory = "/home/${settings.user.admin.username}";
          stateVersion = "24.11";
        };
        programs.home-manager.enable = true;
        imports = [ inputs.flatpaks.homeManagerModules.declarative-flatpak ] ++ lib.filter (path: lib.hasSuffix ".nix" path) (lib.filesystem.listFilesRecursive ../apps);
      };
    };
  };
  ##### Security #####
  users.users.${config.settings.user.admin.username} = {
    isNormalUser = true;
    extraGroups = [ "wheel" "libvirtd" ];
    hashedPasswordFile = "${config.settings.secrets.path}/${config.settings.secrets.hashedPasswordFile}";
    openssh.authorizedKeys.keys = [
      "${config.settings.user.admin.authorizedKey} ${config.settings.user.admin.username}"
    ];
  };
  ##### Auto Login #####
  services.displayManager.autoLogin = {
    enable = config.settings.user.admin.autoLoginEnabled;
    user = config.settings.user.admin.username;
  };
}