{ inputs, config, lib, ... }: {
  imports = [ inputs.home-manager.nixosModules.home-manager ];
  home-manager = {
    useGlobalPkgs = true;
    useUserPackages = true;
    extraSpecialArgs = {
      username = config.variables.user.admin;
    };
    users."${config.variables.user.admin}" = { username, ... }: {
      home = {
        inherit username;
        homeDirectory = "/home/${username}";
        stateVersion = "24.11";
      };
      programs.home-manager.enable = true;
    };
  };
}