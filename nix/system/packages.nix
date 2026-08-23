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
  security.polkit.extraConfig = ''
    polkit.addRule(function(action, subject) {
      if ((action.id == "org.freedesktop.fwupd.refresh-remote" ||
           action.id == "org.freedesktop.fwupd.get-remotes") &&
          subject.user == "fwupd-refresh") {
        return polkit.Result.YES;
      }
    });
  '';
}
