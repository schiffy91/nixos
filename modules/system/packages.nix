{ pkgs, ... }: {
  ##### System Packages ##### 
  environment.systemPackages = (with pkgs; [
    git
    wget
    btrfs-progs
    python3
  ]);
  ##### Unpatched Binaries #####
  programs.nix-ld.enable = true;
}