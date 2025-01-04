{ variables, pkgs, ... }: {
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
  # Declarative Plasma :)
  programs.plasma = {
    enable = true;
    workspace = {
      lookAndFeel = "org.kde.breezedark.desktop";
      cursor.theme = "Bibata-Modern-Ice";
      iconTheme = "Papirus-Light";
      wallpaper = "${pkgs.kdePackages.plasma-workspace-wallpapers}/share/wallpapers/Next/contents/images_dark/5120x2880.png";
    };
    panels = [
      {
        location = "bottom";
        widgets = [
          {
            kickoff = {
              sortAlphabetically = true;
              icon = "nix-snowflake-white";
            };
          }
          {
            iconTasks = {
              launchers = [
                "applications:org.kde.dolphin.desktop"
                "applications:org.kde.konsole.desktop"
              ];
            };
          }
          "org.kde.plasma.marginsseparator"
          {
            digitalClock = {
              calendar.firstDayOfWeek = "sunday";
              time.format = "12h";
            };
          }
          {
            systemTray.items = {
              shown = [
                "org.kde.plasma.networkmanagement"
              ];
              hidden = [
                "org.kde.plasma.volume"
                "org.kde.plasma.battery"
                "org.kde.plasma.bluetooth"
                "org.kde.plasma.clipboard"
              ];
            };
          }
        ];
        hiding = "autohide";
      }
    ];

    window-rules = [
      {
        description = "Dolphin";
        match = {
          window-class = {
            value = "dolphin";
            type = "substring";
          };
          window-types = [ "normal" ];
        };
        apply = {
          maximizehoriz = true;
          maximizevert = true;
        };
      }
    ];

    kwin = {
      edgeBarrier = 0;
      cornerBarrier = false;
      scripts.polonium.enable = true;
      programs.plasma.kwin.titlebarButtons.right = [
        "minimize"
        "maximize"
        "close"
      ];
    };

    kscreenlocker = {
      lockOnResume = true;
      timeout = 10;
    };

    configFile = {
      "kdeglobals"."KScreen"."ScaleFactor" = 1 * variables.desktop.scalingFactor;
      "kdeglobals"."KScreen"."ScreenScaleFactors" = "Virtual-1=${toString (1 * variables.desktop.scalingFactor)};";
    };
  };
}