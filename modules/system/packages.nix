{ pkgs, ... }: {
  ##### System Packages ##### 
  environment.systemPackages = (with pkgs; [
    wget
  ]);
  ##### Unpatched Binaries #####
  programs.nix-ld.enable = true;
}