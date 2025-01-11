{ pkgs, ... }: {
  ##### System Packages ##### 
  environment.systemPackages = (with pkgs; [
    git
    wget
  ]);
  ##### Unpatched Binaries #####
  programs.nix-ld.enable = true;
}