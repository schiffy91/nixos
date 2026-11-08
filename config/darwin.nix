{ config, inputs, ... }:
{
  imports = [ ./pkgs/darwin.nix ./system/darwin/security.nix inputs.home-manager-darwin.darwinModules.home-manager ];
  nix.enable = config.settings.macos.nix.enable;
  nixpkgs.config.allowUnfree = true;
  programs.zsh.enable = config.settings.apps.dev.zsh.enable;
  system.primaryUser = config.settings.users.admin.username;
  users.users.${config.settings.users.admin.username}.home = config.settings.users.admin.homeDirectory;
  home-manager = {
    useGlobalPkgs = true;
    useUserPackages = false;
    backupFileExtension = "before-macos-nix";
    extraSpecialArgs = { inherit inputs; inherit (config) settings; };
  };
  system.stateVersion = 6;
}
