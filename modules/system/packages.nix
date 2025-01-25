{ pkgs, ... }: {
  ##### System Packages ##### 
  environment.systemPackages = (with pkgs; [
    git
    wget
    rsync
  ]);
  ##### Unpatched Binaries #####
  programs.nix-ld.enable = true;
}