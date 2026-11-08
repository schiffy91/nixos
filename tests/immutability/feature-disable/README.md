# Feature-owned persistence

Run through the graph:

```sh
nix-shell tests/shell.nix --run 'NIXOS_CONFIG_ROOT=$PWD ./build/nixosctl graph tests/graph.json run immutability-feature-disable'
```

The parent `persistence-ownership` node checks module merging and enable gates
before this VM runs. The VM evaluates the real rclone and immutability modules
for enabled, disabled and explicitly retained configurations, then exercises
their generated specifications with the production runtime on a Btrfs disk.

It verifies that enabled state persists, disabling quarantines it, re-enabling
does not silently restore the orphan, an explicit recovery restores the data,
and an ordinary host contribution retains the state while the package is off.
The VM unmounts the fixture volume before each reset, matching the initrd's
filesystem boundary. Installed-system boot coverage remains in `install-reset`
and `immutability-reset`.
