{ inputs, config, lib, ... }: {
  imports = [ inputs.home-manager.nixosModules.home-manager ];
  home-manager = {
    useGlobalPkgs = true;
    useUserPackages = true;
    extraSpecialArgs = {
      variables = config.variables;
    };
    users."${config.variables.user.admin}" = { variables, ... }: {
      home = {
        inherit variables;
        homeDirectory = "/home/${variables.user.admin}";
        stateVersion = "24.11";
      };
      programs.home-manager.enable = true;
    };
  };
}