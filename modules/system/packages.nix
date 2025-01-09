{ pkgs, ... }: {
  services.flatpak.enable = true;
  environment.systemPackages = (with pkgs; [
    git
    python313
  ]);
}