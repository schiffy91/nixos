{ ... }: {
  services.flatpak.remotes = { "flathub" = "https://dl.flathub.org/repo/flathub.flatpakrepo"; };
  services.flatpak.packages = [
    "flathub:app/com.visualstudio.code//stable"
  ];
}