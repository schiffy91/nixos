{ pkgs, ... }: {
  ##### Flatpaks ##### 
  services.flatpak.enable = true;
  ##### System Packages ##### 
  environment.systemPackages = (with pkgs; [
    git
  ]);
}