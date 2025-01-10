{ settings, pkgs, ... }: {
  ##### Download Icons #####
  home.packages = with pkgs; [ papirus-icon-theme ];
  ##### Qt #####
/*   qt = {
    enable = true;
    platformTheme = "qtct";
    style.name = "kvantum";
  };
  xdg.configFile = {
    "Kvantum/ArcDark".source = "${pkgs.arc-kde-theme}/share/Kvantum/ArcDark";
    "Kvantum/kvantum.kvconfig".text = "[General]\ntheme=ArcDark";
  }; */
  ##### Download SVG Theme #####
  xdg.dataFile."aurorae/themes/ActiveAccentDark".source = "${pkgs.fetchzip { 
    url = "https://github.com/nclarius/Plasma-window-decorations/archive/02058699173f5651816d4cb31960d08b45553255.zip"; 
    sha256 = "sha256-O4JTtj/q2lJRhWS+nhfQes8jitkrfsSBmENHZb5ioNI=";
  }}/ActiveAccentDark";
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
        library = "org.kde.kwin.aurorae";
        theme = "__aurorae__svg__ActiveAccentDark";
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
                "applications:org.gnome.Nautilus"
                "applications:com.raggesilver.BlackBox"
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
  };
}