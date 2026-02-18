{ settings, pkgs, lib, ... }: lib.mkIf (lib.hasInfix "plasma" settings.desktop.environment) {
  ##### Download Icons #####
  home.packages = with pkgs; [ papirus-icon-theme ];
  programs.plasma = {
    ##### Settings #####
    enable = true;
    overrideConfig = true;
    ##### Workspace #####
    workspace = {
      colorScheme = "BreezeDark";
      cursor = {
        theme = "Breeze";
      };
      wallpaper = "${pkgs.kdePackages.plasma-workspace-wallpapers}/share/wallpapers/Next/contents/images_dark/5120x2880.png";
      iconTheme = "Papirus-Dark";
    };
    ##### Restore #####
    windows.allowWindowsToRememberPositions = false;
    session.sessionRestore.restoreOpenApplicationsOnLogin = "startWithEmptySession";
    ##### Auto Lock #####
    kscreenlocker = {
      autoLock = if settings.user.admin.autoLock.enable then true else false;
      lockOnResume = if settings.user.admin.autoLock.enable then true else false;
      timeout = if settings.user.admin.autoLock.enable then 10 else null;
    };
    ##### Config Files #####
     configFile = {
      kdeglobals = {
        KScreen = {
          ScaleFactor = 1 * settings.desktop.scalingFactor;
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
                "applications:google-chrome.desktop"
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
  };
  ##### Konsole #####
  xdg.configFile."konsolerc".text = ''
    [Desktop Entry]
    DefaultProfile=${settings.user.admin.username}.profile
    [General]
    ConfigVersion=1
    [KonsoleWindow]
    RememberWindowSize=false
    ShowMenuBarByDefault=false
    [MainWindow]
    MenuBar=Disabled
    StatusBar=Disabled
    ToolBarsMovable=Disabled
  '';
  xdg.dataFile."konsole/${settings.user.admin.username}.profile".text = ''
    [Cursor Options]
    CursorShape=2

    [General]
    Name=${settings.user.admin.username}

    [Keyboard]
    KeyBindings=macos

    [Scrolling]
    HistoryMode=2

    [Terminal Features]
    BlinkingCursorEnabled=true

    [KonsoleWindow]
    RememberWindowSize=false
    ShowMenuBarByDefault=false
    [MainWindow]
    MenuBar=Disabled
    StatusBar=Disabled
    ToolBarsMovable=Disabled
  '';
}