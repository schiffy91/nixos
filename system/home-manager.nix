{ inputs, config, pkgs, lib, ...}: {
  imports = [ inputs.home-manager.nixosModules.home-manager ];
  home-manager = {
    extraSpecialArgs = { inherit inputs config pkgs lib; };
    useGlobalPkgs = true;
    useUserPackages = true;
    sharedModules = [ inputs.plasma-manager.homeManagerModules.plasma-manager ];

  };
}