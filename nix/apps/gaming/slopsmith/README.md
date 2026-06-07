Slopsmith is packaged as a Podman Compose wrapper because upstream ships the
application as a self-contained Docker app.

Run it with:

```sh
slopsmith
```

Defaults:

- `DLC_PATH`: Rocksmith 2014's Steam `dlc` directory
- `CONFIG_DIR`: `~/Games/Rocksmith/slopsmith/config`
- `SLOPSMITH_PORT`: `8000`
- `SLOPSMITH_SOURCE`: pinned upstream source from `byrongamatos/slopsmith`

The flake in this folder exposes the same wrapper:

```sh
nix run /etc/nixos/nix/apps/gaming/slopsmith
```
