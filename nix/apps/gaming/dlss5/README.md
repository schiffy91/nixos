# dlss5 — DLSS 5 neural rendering for Proton games

Upstream [DLSS5VKLayer](https://github.com/bmitch87/DLSS5VKLayer), built from source
and unpatched. A Vulkan layer in the game hands each presented frame over shared
memory to a Windows helper that runs NVIDIA's DLSS 5 neural-rendering model, and
puts the processed frame back in the swapchain. No upscaling, no frame-rate gain:
a fixed per-frame cost for the look.

## Pieces

- `package.nix` — layer `.so` + implicit-layer manifest, MinGW-built `dlssnr_helper.exe`,
  `dlssnr-shmctl`, the upstream `dlssnr-helper` script, and the Qt GUI.
- `default.nix` — the module: layer manifest into `/run/opengl-driver`, the helper as
  `dlss5-helper.service` (user unit, off by default), `dlss5ctl`, and the tray.
- `tray.btrc` — the btrc tray: one check item that starts/stops the helper, style presets,
  and the helper log.

## How the toggle works

The layer is fail-open: with `VKLayer_DLSS5=1` in a game's environment it loads and, when no
helper answers, presents the game's own frames and retries every few seconds. So the launch
environment never changes; the tray starts or stops `dlss5-helper.service` and running games
pick it up or drop it mid-session.

Games opt in per entry in `steam/games.nix` with `launchPrefix = dlss5LaunchEnv;`
(`VKLayer_DLSS5=1 DLSSNR_SHM=…`). The shared-memory file lives under `$HOME` because
pressure-vessel gives every game a private `/tmp`.

## The helper's runner

proton-custom's wine binaries are GE's build, so the helper runs through `umu-run` inside the
Steam runtime container, exactly like Battle.net. The prefix is `~/.local/share/dlssnr/prefix/wine`.

## The one thing not packaged

`nvngx_dlssnr.dll` (NVIDIA's model runtime) is not in any SDK and is not shipped here. Drop it
into `~/.local/share/dlssnr/binaries/` and restart the helper. Until then the helper reports
"neural disabled" and games render untouched.

## Removal

Set `settings.apps.dlss5.enable = false` (or delete this folder and the setting), drop the
`games.nix` entries that use `dlss5LaunchEnv`, and remove `~/.config/dlssnr`,
`~/.local/state/dlssnr`, `~/.local/share/dlssnr`.

## Not done yet

- 32-bit games: the 32-bit layer is not built (needs an i686 build of `layer_linux`).
- Per-game selection at runtime: upstream has no allowlist by executable name; until that lands
  the choice is per `games.nix` entry, and Battle.net titles are all-or-nothing.
