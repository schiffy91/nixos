{ pkgs, ... }: {
  ##### System Packages ##### 
  environment.systemPackages = (with pkgs; [
    git
  ]);
  ##### Unpatched Binaries #####
  programs.nix-ld.enable = true;
}