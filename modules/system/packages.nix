{ pkgs, ... }: {
  ##### System Packages ##### 
  environment.systemPackages = (with pkgs; [
    git
    wget
    nix-direnv
  ]);
  ##### Unpatched Binaries #####
  programs.nix-ld.enable = true;
}