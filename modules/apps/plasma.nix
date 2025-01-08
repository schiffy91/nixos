{ settings, pkgs, ... }: {
    programs.plasma = {
    enable = true;
    workspace = {
      lookAndFeel = "org.kde.breezedark.desktop";
      cursor.theme = "Bibata-Modern-Ice";
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
                "org.kde.plasma.brightness"
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
    kscreenlocker = {
      lockOnResume = true;
      timeout = 10;
    };
    configFile = {
      "kdeglobals"."KScreen"."ScaleFactor" = 1 * settings.desktop.scalingFactor;
      "kdeglobals"."KScreen"."ScreenScaleFactors" = "Virtual-1=${toString (1 * settings.desktop.scalingFactor)};";
      "kwinrc"."Xwayland"."Scale" = 1 * settings.desktop.scalingFactor;
    };
  };
}