{ pkgs, ... }: {
  ##### System Packages ##### 
  environment.systemPackages = (with pkgs; [
    git
    wget
    btrfs-progs
  ]);
  ##### Unpatched Binaries #####
  programs.nix-ld.enable = true;
}