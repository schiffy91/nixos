{ settings, pkgs, lib, ... }: {
  home.packages = with pkgs; [ papirus-icon-theme ];
  home.file.".local/share/aurorae/themes/ActiveAccentDark".source =
    let 
      decoration = pkgs.fetchzip { 
        url = "https://github.com/nclarius/Plasma-window-decorations/archive/02058699173f5651816d4cb31960d08b45553255.zip";
        hash = "sha256-1lm0caz6ais3k20w8zibv6527kvss0brxgk4hm8m5npa7yv570iv";
      }; 
    in
      "${decoration}/Plasma-window-decorations-main/ActiveAccentDark";
  programs.plasma = {
    enable = true;
    workspace = {
      lookAndFeel = "org.kde.breezedark.desktop";
      cursor.theme = "Bibata-Modern-Ice";
      wallpaper = "${pkgs.kdePackages.plasma-workspace-wallpapers}/share/wallpapers/Next/contents/images_dark/5120x2880.png";
      iconTheme = "Papirus-Dark";
      windowDecorations.theme = "__aurorae__svg__ActiveAccentDark";
      windowDecorations.library = "org.kde.kwin.aurorae";
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
      autoLock = if settings.user.admin.autoLockEnabled then true else false;
      lockOnResume = if settings.user.admin.autoLockEnabled then true else false;
      timeout = if settings.user.admin.autoLockEnabled then 10 else null;
    };
    configFile = {
      "kdeglobals"."KScreen"."ScaleFactor" = 1 * settings.desktop.scalingFactor;
      "kdeglobals"."KScreen"."ScreenScaleFactors" = "Virtual-1=${toString (1 * settings.desktop.scalingFactor)};";
      "kwinrc"."Xwayland"."Scale" = 1 * settings.desktop.scalingFactor;
    };
  };
}