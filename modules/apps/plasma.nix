{ settings, pkgs, ... }: {
  home.packages = with pkgs; [ papirus-icon-theme ];
  xdg.dataFile."aurorae/themes/ActiveAccentDark".source = "${pkgs.fetchzip { 
    url = "https://github.com/nclarius/Plasma-window-decorations/archive/02058699173f5651816d4cb31960d08b45553255.zip"; 
    sha256 = "sha256-O4JTtj/q2lJRhWS+nhfQes8jitkrfsSBmENHZb5ioNI=";
  }}/ActiveAccentDark";
  programs.plasma = {
    enable = true;
    workspace = {
      lookAndFeel = "org.kde.breezedark.desktop";
      cursor.theme = "Bibata-Modern-Ice";
      wallpaper = "${pkgs.kdePackages.plasma-workspace-wallpapers}/share/wallpapers/Next/contents/images_dark/5120x2880.png";
      iconTheme = "Papirus-Dark";
      windowDecorations.theme = "ActiveAccentDark";
      windowDecorations.library = "org.kde.kwin.aurorae";
      cursor.size = builtins.floor (24 * settings.desktop.scalingFactor);
    };
    panels = [
      {
      location = "top";
      height = 26;
      floating = true;
      widgets = [
        {
          applicationTitleBar = {
            layout.elements = [];
            windowControlButtons = {
              iconSource = "breeze";
              buttonsAspectRatio = 95;
              buttonsMargin = 0;
            };
            windowTitle = {
              source = "appName";
              hideEmptyTitle = true;
              undefinedWindowTitle = "";
              margins = {
                left = 5;
                right = 5;
              };
            };
            overrideForMaximized = {
              enable = true;
              elements = ["windowCloseButton" "windowMaximizeButton" "windowMinimizeButton" "windowIcon" "windowTitle"];
              source = "appName";
            };
          };
        }
        "org.kde.plasma.appmenu"
        "org.kde.plasma.panelspacer"
        {
          digitalClock = {
            date = {
              enable = true;
              position = "besideTime";
            };
            time.showSeconds = "always";
          };
        }
        "org.kde.plasma.panelspacer"
        {
          systemTray = {
            icons.scaleToFit = true;
            items = {
              shown = [
                "org.kde.plasma.battery"
              ];
              configs.battery.showPercentage = true;
            };
          };
        }
      ];
    }
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
    ##### Windows: Title Bar Buttons ##### 
    kwin.titlebarButtons = {
      left = [ ];
      right = [
        "minimize"
        "maximize"
        "close"
      ];
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
        KScreen.ScaleFactor = 1 * settings.desktop.scalingFactor;
      };
      kwinrc = {
        Xwayland = {
          Scale = 1 * settings.desktop.scalingFactor;
        };
      };
    };
  };
}