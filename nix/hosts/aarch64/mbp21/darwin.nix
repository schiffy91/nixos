{ config, pkgs, ... }:
{
  nixpkgs.hostPlatform = "aarch64-darwin";
  home-manager.users.${config.settings.users.admin.username}.imports = [ ./home.nix ];
  settings = {
    apps = {
      packages = with pkgs; [ _1password-cli coreutils direnv gh glfw imagemagick nodejs podman python313 qemu qpdf wgpu-native ];
      zsh = {
        enable = true;
        shellAliases = {
          nix-update = "sudo darwin-rebuild switch --flake ~/Drive/dev/nixos#mbp21";
          nix-upgrade = "nix flake update nixpkgs-darwin home-manager-darwin nix-darwin --flake ~/Drive/dev/nixos && sudo darwin-rebuild switch --flake ~/Drive/dev/nixos#mbp21";
        };
        initContent = ''
          export PATH="$PATH:/Users/alexanderschiffhauer/.lmstudio/bin"
          . "$HOME/.local/bin/env"
        '';
        profileExtra = builtins.readFile ./profile.zsh;
      };
      git.includes = [ "${config.settings.users.admin.homeDirectory}/.config/git/local-before-nix.gitconfig" ];
      gemini.enable = true;
      agents = {
        skills = [ "engineering-decomposition" "taste" "humanizer" "project-structure-taste" ];
        rules = true;
      };
    };
    macos.permissions = import ./permissions.nix;
  };
}
