# Offline folders

Run through the graph:

```sh
nix-shell tests/shell.nix --run 'NIXOS_CONFIG_ROOT=$PWD ./build/nixosctl graph tests/graph.json run rclone-offline-folder'
```

The VM runs the user units the production rclone module generates for one
`settings.apps.utils.rclone.offline` folder. `gdrive:` is an rclone alias for a
local folder, so the sync itself is the real `rclone bisync` with the module's
flags and no network.

It verifies that nothing runs without the marker file, that the first run copies
what either side lacks and lets the newer file win without deleting anything,
that later runs carry changes and deletions both ways, that a file changed on
both sides keeps both versions, that a side missing more than a tenth of its
files stops the sync until the files are back, and that an emptied folder
without its marker never reaches the Drive.
