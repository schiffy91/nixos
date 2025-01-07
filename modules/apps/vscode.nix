{ config, pkgs, unstable-pkgs, ... }:
{
  home.packages = [
    #unstable-pkgs.vscode
    unstable-pkgs.vscode.fhs
  ];
}