{ pkgs, ... }: {
  ##### Flatpaks ##### 
  services.flatpak = {
    enable = true;
    remotes = { "flathub" = "https://dl.flathub.org/repo/flathub.flatpakrepo"; };
  };
  ##### System Packages ##### 
  environment.systemPackages = (with pkgs; [
    wget
    git
  ]);
  ##### Unpatched Binaries #####
  programs.nix-ld.enable = true;
}