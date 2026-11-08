# Per-app Steam config. Travels with the app — true wherever it's installed.
# Apps not listed here inherit proton-custom + the default Steam Play env.
{ protonCustomName, rsSampleSize, rsSampleRate, assetto }:
{
  "221680" = {  # Rocksmith 2014 — ASIO + low-latency pipewire
    compatTool = protonCustomName;
    launchOptions = "LD_PRELOAD=/usr/lib32/libjack.so PIPEWIRE_LATENCY=${toString rsSampleSize}/${toString rsSampleRate} %command%";
  };
  "3240220" = {  # GTA V Enhanced
    launchPrefix = "SteamDeck=1";  # keeps the Social Club launcher off its desktop path
  };
} // (if assetto.enabled then {
  "244210" = assetto.steamConfig; # Content Manager + CSP, via XWayland.
} else {})
