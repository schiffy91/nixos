{ settings, pkgs, lib, ... }: {
  ##### Download Icons #####
  home.packages = with pkgs; [ papirus-icon-theme ];
  programs.plasma = {
    enable = true;
  };

  # Apply scaling settings for X11 and Wayland
  environment.sessionVariables = lib.mkIf (lib.hasInfix "plasma" settings.desktop.environment) {
      # Wayland
      "QT_AUTO_SCREEN_SCALE_FACTOR" = "1";

      # X11: Use QT scaling
      "PLASMA_USE_QT_SCALING" = "1";
      "QT_SCALE_FACTOR" = lib.mkIf (settings.desktop.environment == "plasma-x11") (toString settings.desktop.scalingFactor);
      
      # KWin XWayland (For X11 apps under Wayland, probably unneeded in X11 session)
      "KWIN_X11_SCALE_FACTOR" = toString settings.desktop.scalingFactor; 
    };
  /*xdg.configFile."breezerc".text = ''
  [Common]
  OutlineCloseButton=true
  '';
  programs.plasma = {
    ##### Settings #####
    enable = true;
    overrideConfig = true;
    ##### Workspace #####
    workspace = {
      colorScheme = "BreezeDark";
      cursor = {
        theme = "Breeze";
        size = builtins.floor (24 * settings.desktop.scalingFactor);
      };
      wallpaper = "${pkgs.kdePackages.plasma-workspace-wallpapers}/share/wallpapers/Next/contents/images_dark/5120x2880.png";
      iconTheme = "Papirus-Dark";
      windowDecorations = {
        library = "org.kde.breeze";
        theme = "Breeze";
      };
    };
    ##### Restore #####
    windows.allowWindowsToRememberPositions = false;
    session.sessionRestore.restoreOpenApplicationsOnLogin = "startWithEmptySession";
    ##### Auto Lock #####
    kscreenlocker = {
      autoLock = if settings.user.admin.autoLockEnabled then true else false;
      lockOnResume = if settings.user.admin.autoLockEnabled then true else false;
      timeout = if settings.user.admin.autoLockEnabled then 10 else null;
    };
    ##### Config Files #####
    configFile = {
      kdeglobals = {
        KScreen = { 
          ScaleFactor = 1 * settings.desktop.scalingFactor;
          ScreenScaleFactors = "Virtual-1=${toString (1 * settings.desktop.scalingFactor)};";
        };
        General = {
          AccentColor = "40,40,40";
        };
      };
      kwinrc = {
        Xwayland = {
          Scale = 1 * settings.desktop.scalingFactor;
        };
      };
    };
    ##### Start Menu #####
    panels = [
      {
        location = "bottom";
        hiding = "autohide";
        floating = true;
        widgets = [
          {
            kickoff = {
              sortAlphabetically = true;
              icon = "nix-snowflake";
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
      }
    ];
  };*/
}