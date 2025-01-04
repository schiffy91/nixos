{ variables, pkgs, lib, ... }: {
  home = {
    username = variables.user.admin;
    homeDirectory = "/home/${variables.user.admin}";
    stateVersion = "24.11";
  };
  programs.home-manager.enable = true;
  # Configure 1Password SSH Agent to add all my SSH Keys
  xdg.configFile."1Password/ssh/agent.toml".text = ''
      [[ssh-keys]]
      vault = "Private"
    '';
  programs.plasma = {
    enable = true;
    workspace = {
      lookAndFeel = "org.kde.breezedark.desktop";
      cursor.theme = "Bibata-Modern-Ice";
      iconTheme = "Papirus-Dark";
      wallpaper = "${pkgs.kdePackages.plasma-workspace-wallpapers}/share/wallpapers/Patak/contents/images/1080x1920.png";
    };

    panels = [{
      location = "bottom";
      widgets = [
        "org.kde.plasma.kickoff"
        "org.kde.plasma.icontasks"
        "org.kde.plasma.marginsseparator"
        "org.kde.plasma.systemtray"
        "org.kde.plasma.digitalclock"];
      }
    ];
    configFile = {
      "kdeglobals"."KScreen"."ScaleFactor" = (builtins.toString 1 * variables.desktop.scalingFactor);
      "kdeglobals"."KScreen"."ScreenScaleFactors" = "Virtual-1=${builtins.toString 1 * variables.desktop.scalingFactor};";
    };
  };
}