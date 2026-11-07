This scenario runs a disposable OVMF VM through actual signed bootloader boots.
It generates fresh test-only keys inside the guest and never reads host keys.

It verifies authenticated platform-key removal with pinned sbctl, wrong-key
rejection, signed maintenance with automatic enrollment disabled and stale ESP
enrollment assets removed, only maintenance/recovery generations retained, two
boots remaining in Setup Mode, and explicit re-enrollment of PK without rewriting
KEK/db. It boot-tests signed recovery before removal, with PK removed, and after
re-enrollment. The ordinary signed maintenance generation remains the default
after one-shot recovery.

Run through the graph:

```
make build
nix-shell tests/shell.nix --run 'NIXOS_CONFIG_ROOT=$PWD ./build/nixosctl graph tests/graph.json run secure-boot-removal'
```

This is a verification of the key-removal procedure, not a new `nixosctl`
command. It does not prove Deck firmware behavior, firmware update compatibility,
independent backup restoration, or bootability of a physical recovery USB.
