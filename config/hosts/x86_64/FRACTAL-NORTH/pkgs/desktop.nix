{ config, lib, ... }:
let
  apps = config.settings.apps;
  chromeApps = apps.utils.chrome.webApps;
in {
  settings.apps.utils.chrome.webApps = import ../../../../pkgs/utils/chrome-apps.nix;
  settings.desktop.plasma.shortcuts = {  # the keyboard's Fn never reaches Linux, so Meta stands in
    brightness-down = { key = "Meta+F9"; command = "nixosctl displays brightness down 5 150"; };  # percent a step, ms between steps while held
    brightness-up = { key = "Meta+F10"; command = "nixosctl displays brightness up 5 150"; };
  };
  settings.desktop.plasma.taskbarLaunchers = [
    "applications:org.kde.dolphin.desktop"
  ] ++ lib.optional apps.utils.chrome.enable "applications:google-chrome.desktop"
    ++ map (app: app.launcher) (lib.filter (app: app.enable) [ chromeApps.gmail chromeApps.calendar chromeApps.whatsapp ])
    ++ lib.optional apps.utils.cider.enable "applications:cider-2.desktop"
    ++ lib.optional (apps.dev.vscode.enable) "applications:code.desktop"
    ++ [ "applications:org.kde.konsole.desktop" ]
    ++ lib.optional apps.gaming.battlenet.enable "applications:battlenet.desktop"
    ++ lib.optional apps.gaming.steam.enable "applications:steam.desktop";
}
