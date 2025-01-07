{ config, pkgs, unstable-pkgs, ... }:
{
  home.packages = [
    unstable-pkgs.vscode
    unstable-pkgs.vscode.fhs
    pkgs.nx-ld
  ];
  #programs.nix-ld.enable = true; # See https://nixos.wiki/wiki/Visual_Studio_Code#Remote_SSH 
}