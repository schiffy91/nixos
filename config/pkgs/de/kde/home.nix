{ settings, lib, pkgs, ... }:
let
  primary = lib.findFirst (o: o.primary) null settings.desktop.outputs;
  primaryScale = if primary == null then 1.0 else primary.scaleFactor;
  scaleFactorsString = lib.concatMapStringsSep "" (o: "${o.name}=${toString o.scaleFactor};") settings.desktop.outputs;
  miceConfig = lib.listToAttrs (map (m: {
    name = "Libinput][${m.vendorId}][${m.productId}][${m.name}";
    value = {
      PointerAccelerationProfile = if m.accelProfile == "flat" then 1 else 2;
      PointerAcceleration = "0.000";
    };
  }) settings.input.libinputMice);
in {
  ##### Icon Theme #####
  home.packages = [ settings.desktop.plasma.iconThemePackage ];
  xdg.desktopEntries.desktop-keyboard = lib.mkIf settings.desktop.plasma.virtualKeyboard.enable {
    name = "On-Screen Keyboard";
    exec = "${pkgs.kdePackages.qttools}/bin/qdbus org.kde.KWin /VirtualKeyboard org.kde.kwin.VirtualKeyboard.forceActivate";
    icon = "input-keyboard-virtual";
    categories = [ "Utility" "Accessibility" ];
    terminal = false;
  };
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
      autoLock = settings.users.admin.autoLock.enable;
      lockOnResume = settings.users.admin.autoLock.enable;
      timeout = if settings.users.admin.autoLock.enable then 10 else null;
    };
    ##### Config Files #####
     configFile = {
      kdeglobals = {
        KScreen = {
          ScaleFactor = primaryScale;
          ScreenScaleFactors = scaleFactorsString;
        };
        General.AccentColor = settings.desktop.plasma.accentColor;
      };
      kwinrc = {
        Xwayland.Scale = primaryScale;
        Windows.Placement = "UnderMouse";
        Wayland = {
          DisableTonemapping = true;
        } // lib.optionalAttrs settings.desktop.plasma.virtualKeyboard.enable {
          InputMethod = "${pkgs.kdePackages.plasma-keyboard}/share/applications/org.kde.plasma.keyboard.desktop";
          VirtualKeyboardMode = 1; # Touch/tablet input; the launcher can also force it open.
        };
      };
      kcminputrc = miceConfig;
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
              launchers = settings.desktop.plasma.taskbarLaunchers;
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
                "org.kde.plasma.volume"
              ];
              hidden = [
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
    defaultProfile = settings.users.admin.username;
    profiles.${settings.users.admin.username} = {
      name = settings.users.admin.username;
      font.name = "Hack";
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
