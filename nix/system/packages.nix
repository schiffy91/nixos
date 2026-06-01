{ pkgs, lib, ... }:
let
  hasAppImageRun = lib.meta.availableOn pkgs.stdenv.hostPlatform pkgs.appimage-run;
in {
  ##### System Packages #####
  environment.systemPackages = (with pkgs; [
    git
    wget
  ]) ++ lib.optionals hasAppImageRun (with pkgs; [
    appimage-run
  ]);
  programs = {
    ##### Unpatched Binaries #####
    nix-ld.enable = true;
    ##### AppImage #####
    appimage = lib.mkIf hasAppImageRun {
      binfmt = true;
    };
    ##### direnv #####
    direnv.enable = true;
  };
  ##### Firmware #####
  services.fwupd.enable = true;
}
