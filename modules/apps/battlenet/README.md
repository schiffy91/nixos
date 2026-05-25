# Battle.net Dev Loop

This directory owns Battle.net application workflows. The Proton compat-tool
source and packaging live in `modules/apps/pkg-overrides/proton-custom`.

The KDE desktop entry and every script here launch the same command:

```bash
battlenet
```

That wrapper is defined by `modules/apps/battlenet.nix`.

## Quick Loop

Use the Makefile so build, install, configure, run, and log collection stay
repeatable:

```bash
./bin/battlenet-make cycle BUILD_MODE=changed
```

The cycle:

1. Builds only changed `proton-custom` Wine/DXVK targets.
2. Points Steam's `scwhine-GE-Proton10-34` compat-tool symlink at the writable
   dev copy.
3. Configures the Battle.net prefix for that compat tool.
4. Runs `battlenet`.
5. Prints narrowed logs and SNI metadata.

Use `cycle-no-run` to stop after build/install/configure.

## Diagnostics

Default diagnostics are log and D-Bus based. Full-desktop screenshots are not
available from these scripts.

Useful targets:

```bash
./bin/battlenet-make status
./bin/battlenet-make logs LOG_LINES=80
./bin/battlenet-make sni
./bin/battlenet-make context-menu
./bin/battlenet-make cleanup
```

`capture-window` and `frames-window` exist only for active Battle.net window
captures when visual timing bugs require it.

Expected runtime signals:

- Wrapper log says `scwhine DEV GE-Proton10-34 (Wayland SNI)` in dev mode.
- Proton log options include `wayland` and `hdr`.
- Battle.net command line does not include `--use-angle=desktop` or
  `--disable-gpu-compositing`.
- CEF renderer receives `--force-device-scale-factor=<display scale>`.
- StatusNotifierItem metadata contains title `Battle.net` and icon name
  `battlenet`.
- `context-menu` returns promptly.
