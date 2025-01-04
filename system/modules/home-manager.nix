{ inputs, config, lib, ... }: inputs.home-manager.nixosModules.home-manager lib {
  home-manager.useGlobalPkgs = true;
  home-manager.useUserPackages = true;
  home-manager.users."${config.variables.user.admin}" = import ../users/admin.nix;
}