{ settings, lib, ... }:
let chromeApp = id: "applications:chrome-${id}-Default.desktop"; in
lib.mkIf (lib.hasInfix "plasma" settings.desktop.environment) {
  ##### Icon Theme #####
  home.packages = [ settings.desktop.plasma.iconThemePackage ];
  programs.plasma = {
    ##### Settings #####
    enable = true;
    overrideConfig = true;
    ##### Workspace #####
    workspace = {
      colorScheme = settings.desktop.plasma.colorScheme;
      cursor = {
        theme = settings.desktop.cursor.theme;
      };
      wallpaper = settings.desktop.plasma.wallpaper;
      iconTheme = settings.desktop.plasma.iconTheme;
    };
    ##### Restore #####
    windows.allowWindowsToRememberPositions = false;
    session.sessionRestore.restoreOpenApplicationsOnLogin = "startWithEmptySession";
    ##### Auto Lock #####
    kscreenlocker = {
      autoLock = settings.user.admin.autoLock.enable;
      lockOnResume = settings.user.admin.autoLock.enable;
      timeout = if settings.user.admin.autoLock.enable then 10 else null;
    };
    ##### Config Files #####
     configFile = {
      kdeglobals = {
        KScreen = {
          ScaleFactor = settings.desktop.scalingFactor;
          ScreenScaleFactors = "${settings.desktop.primaryOutput}=${toString settings.desktop.scalingFactor};";
        };
        General.AccentColor = settings.desktop.plasma.accentColor;
      };
      kwinrc = {
        Xwayland.Scale = settings.desktop.scalingFactor;
        # New windows spawn under the cursor — i.e. on whichever monitor's taskbar you clicked.
        Windows.Placement = "UnderMouse";
      };
    };
    ##### Start Menu #####
    panels = [
      {
        location = "bottom";
        hiding = "autohide";
        floating = true;
        screen = "all";
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
                (chromeApp "fmgjjmmmlfnkbppncabfkddbjimcfncm")  # Gmail
                (chromeApp "kjbdgfilnfhdoflbpgamdcdgpehopbep")  # Calendar
                (chromeApp "hnpfjngllnobngcgfapefoaidbinmjnm")  # WhatsApp
                "applications:cider-2.desktop"
                "applications:code.desktop"
                "applications:org.kde.konsole.desktop"
                "applications:steam.desktop"
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
  programs.konsole = {
    enable = true;
    defaultProfile = settings.user.admin.username;
    profiles.${settings.user.admin.username} = {
      name = settings.user.admin.username;
      extraConfig = {
        "Cursor Options".CursorShape = 2;
        "Keyboard".KeyBindings = "macos";
        "Scrolling".HistoryMode = 2;
        "Terminal Features".BlinkingCursorEnabled = true;
      };
    };
    extraConfig = {
      "KonsoleWindow".RememberWindowSize = false;
      "KonsoleWindow".ShowMenuBarByDefault = false;
      "KonsoleWindow".ToolBarsMovable = false;
      "MainWindow".MenuBar = "Disabled";
      "MainWindow".StatusBar = "Disabled";
    };
  };
}