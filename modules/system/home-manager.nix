{ inputs, config, lib, ... }: {
  imports = [ inputs.home-manager.nixosModules.home-manager ];
  home-manager = {
    useGlobalPkgs = true;
    useUserPackages = true;
    extraSpecialArgs = { variables = config.variables; };
    users."${config.variables.user.admin}" = import ../homes/admin.nix;
  };
}