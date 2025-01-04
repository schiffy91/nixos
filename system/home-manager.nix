{ inputs, config, ... }: inputs.home-manager.nixosModules.home-manager {
  home-manager.useGlobalPkgs = true;
  home-manager.useUserPackages = true;
  home-manager.users."${config.variables.user.admin}" = import ./users/admin.nix;
}