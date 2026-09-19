# Per-app Steam config. Travels with the app — true wherever it's installed.
# Apps not listed here inherit proton-custom + the default Steam Play env.
{ protonCustomName, chromiumDpi, rsSampleSize, rsSampleRate }:
{
  "221680" = {  # Rocksmith 2014 — ASIO + low-latency pipewire
    compatTool = protonCustomName;
    launchOptions = "LD_PRELOAD=/usr/lib32/libjack.so PIPEWIRE_LATENCY=${toString rsSampleSize}/${toString rsSampleRate} %command%";
  };
  "3240220" = {  # GTA V Enhanced — Social Club launcher is Chromium
    launchPrefix = "SteamDeck=1";  # keeps the launcher off its desktop path
    launchSuffix = chromiumDpi;
  };
  "1174180" = {  # Red Dead Redemption 2 — Rockstar launcher is Chromium
    launchSuffix = chromiumDpi;
  };
  "1091500" = {  # Cyberpunk 2077 — REDlauncher is Chromium
    launchSuffix = chromiumDpi;
  };
}
