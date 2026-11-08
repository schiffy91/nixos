Slopsmith is packaged with a BTRC Podman Compose controller because upstream ships the
application as a self-contained Docker app.

Run it with:

```sh
slopsmith
```

Defaults:

- `DLC_PATH`: Rocksmith 2014's Steam `dlc` directory
- `CONFIG_DIR`: `~/Games/Rocksmith/slopsmith/config`
- `SLOPSMITH_PORT`: `8000`
- `SLOPSMITH_SOURCE`: pinned upstream source from `schiffy91/slopsmith`

The repository flake exposes the same launcher:

```sh
nix run /etc/nixos#slopsmith
```
