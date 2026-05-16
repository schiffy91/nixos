{ inputs, config, lib, ... }:
let
  nixFiles = dir:
    let e = builtins.readDir dir; in
    lib.attrValues (lib.mapAttrs (n: _: dir + "/${n}")
      (lib.filterAttrs (n: t: t == "regular" && lib.hasSuffix ".nix" n) e));
in {
  imports = [
    inputs.home-manager.nixosModules.home-manager
  ] ++ nixFiles ../apps;

  users.mutableUsers = false;
  users.users.${config.settings.user.admin.username} = {
    isNormalUser = true;
    extraGroups = [ "wheel" "networkmanager" ];
    hashedPasswordFile = "${config.settings.secrets.path}/${config.settings.secrets.hashedPasswordFile}";
    openssh.authorizedKeys.keys = config.settings.user.admin.authorizedKeys;
  };

  services.displayManager.autoLogin = {
    enable = config.settings.user.admin.autoLogin.enable;
    user = config.settings.user.admin.username;
  };

  home-manager = {
    useGlobalPkgs = true;
    useUserPackages = true;
    backupFileExtension = "hm-backup";
    sharedModules = [ inputs.plasma-manager.homeModules.plasma-manager ];
    extraSpecialArgs = { settings = config.settings; };
    users.${config.settings.user.admin.username} = { settings, ... }: {
      home = {
        username = settings.user.admin.username;
        homeDirectory = "/home/${settings.user.admin.username}";
        stateVersion = "24.11";
      };
      programs.home-manager.enable = config.settings.user.admin.homeManager.enable;
      imports = nixFiles ../apps/home-manager;
    };
  };
}
