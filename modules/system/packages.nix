{ pkgs, ... }: {
  ##### System Packages #####
  environment.systemPackages = (with pkgs; [
    git
    wget
    appimage-run
  ]);
  ##### Unpatched Binaries #####
  programs.nix-ld.enable = true;
  ##### direnv #####
  programs.direnv.enable = true;
  ##### Firmware #####
  services.fwupd.enable = true;
}