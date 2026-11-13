# Application artwork

KDE uses application icons. Steam's library in Gaming Mode needs four separate
assets as well: a 600×900 portrait cover, a 920×430 landscape cover for Recent
Games, a 1920×620 or 3840×1240 hero background, and a transparent title logo.
Gamescope itself does not manage these images; the Steam client does.

`package.nix` packages complete, curated SteamGridDB sets for every declared
non-Steam app. `sources.json` records each original image URL, SHA-256, dimensions,
author, and attribution page. Downloads are fixed-output Nix dependencies, with
no API key, scraper, or network access at login. Images keep their original
composition and resolution; no generic icon-to-banner generator remains.

| App | Artwork source | Selection |
| --- | --- | --- |
| Semu | [ES-DE](https://www.steamgriddb.com/game/5323156) | Illustrated retro cover, current ES-DE wordmark, console photography hero |
| Battle.net | [Battle.net](https://www.steamgriddb.com/game/5248250) | Blue branded covers, character panorama, transparent Battle.net wordmark |
| Chrome | [Google Chrome](https://www.steamgriddb.com/game/37126) | Light branded covers, offset Chrome hero, readable dark wordmark |
| Moonlight | [Moonlight Game Streaming](https://www.steamgriddb.com/game/5323697) | Coordinated OmniDeath lunar covers, starfield hero and title |
| chiaki-ng | [chiaki-ng](https://www.steamgriddb.com/game/5441527) | Coordinated LXVI lime/controller-symbol set and white title |

Deck Pad (deckpad) is the exception: SteamGridDB has no art of a Deck
acting as a Steam Controller. `deckpad/compose.py` builds its set from the
pictures on [Valve's Steam Controller page](https://store.steampowered.com/hardware/steamcontroller):
the controller on kraft paper for the hero, the product photograph placed on
the same paper for both covers, and Valve's controller glyph for the logo and
the icon. The five PNGs it writes are committed in `deckpad/`, because Valve
replaces the files behind that page; run the script to redo them.

Steam games such as Assetto Corsa and Rocksmith already receive their library
artwork from Steam. They do not need duplicate non-Steam shortcuts.

## Desktop icons

`icons.nix` uses the packaged upstream icons for ES-DE, Moonlight, chiaki-ng and
Chrome. Steam gets PNGs because its shortcut icon loader does not handle SVGs.
KDE keeps the native application icons; Semu publishes ES-DE's scalable SVG as
`es-de`, matching its desktop entry and keeping it sharp at high DPI.

`battlenet.png` is Blizzard's 256px application icon, extracted from Battle.net
Launcher.exe with icoutils. It is available before the first Wine-prefix setup.
The original artwork and trademarks remain their respective owners' property;
SteamGridDB contributor attribution is recorded per asset in `sources.json`.

## Choosing different artwork

The tools serve different workflows:

- [SGDBoop](https://www.steamgriddb.com/boop) is installed with Steam. In KDE, enable
  SGDBoop on SteamGridDB and use an image's **non-Steam** Boop button to choose
  the existing shortcut. Pick both cover orientations, a hero and a logo.
- [SteamGridDB for Decky](https://github.com/SteamGridDB/decky-steamgriddb) lets you
  change art within Gaming Mode. The Deck already has Decky Loader; install
  SteamGridDB from its plugin store. This repository does not install that plugin.
- [Steam ROM Manager](https://github.com/SteamGridDB/steam-rom-manager) imports
  individual ROMs and manages their Steam art. Our existing Semu shortcut opens
  the whole library, so SRM is not needed to populate these launchers.
- Inside ES-DE, use its [scraper and theme
  downloader](https://gitlab.com/es-de/emulationstation-de/-/blob/master/USERGUIDE.md).
  ScreenScraper/TheGamesDB media and ES-DE themes are independent of Steam art.
  The Deck's existing Semu paths keep ROM media on the SD card; changing these
  launcher assets does not scrape ROMs or replace the user's ES-DE theme.

The shared Steam module declares shortcuts only for enabled apps. Names and
executables remain stable to preserve IDs, controller settings and play history.
The boot service installs artwork while Steam is stopped. After rebuilding,
reboot into Gaming Mode, or close Steam and run `sudo systemctl start steam-shortcuts`.

`grid/.nixos-artwork/` records the hashes of installed defaults. Subsequent runs
upgrade those files only while their bytes still match. Custom covers, heroes and
logos in PNG or alternate JPEG/WebP files are preserved; shortcut icons remain
declaratively configured. `legacy.json` identifies the exact PNG outputs of
the former gradient generator (including its icon-as-logo files) so those can be
replaced without deleting unrelated artwork. Unrecognized older variants stay
untouched; SGDBoop or Decky can replace them explicitly.

To update the defaults, select and visually review a complete set on SteamGridDB,
then update the corresponding entries in `sources.json` with the original PNG
download URLs and hashes (`nix store prefetch-file --json URL`). Keep the source
page and artist attribution. `tests/gaming/artwork.nix` validates all 20 downloaded
images and the four composed for deckpad; the Steam installer tests cover upgrades, legacy migration and custom-art
preservation.
