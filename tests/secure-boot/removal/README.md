This scenario exercises `nixosctl secure-boot prepare`, `enable`, `disable`, and
`status` in a disposable OVMF VM. Fresh test keys are generated inside the guest;
host keys and firmware are never used for enrollment.
The VM has a measured virtual TPM and no unused NIC PXE ROM, so sbctl's normal
Option ROM checks run without bypass flags.

The fixture exposes its prebuilt NixOS specializations through a small local
flake. It runs the actual packaged CLI, Nix evaluator, `nixos-rebuild`, activation,
Lanzaboote, sbctl and firmware. This keeps the VM offline without substituting
commands at the signing or firmware boundary. A separately signed recovery
specialization supplies the configured recovery paths checked by the CLI. Its
actual recovery EFI entry is booted before removal, with PK absent, and after
re-enrollment.

The checks cover:

- Preparation without firmware enrollment, complete-key validation and key reuse.
- Required recovery signatures before enrollment, and idempotent enable.
- Refusal of foreign local keys before rebuilding or altering firmware.
- Refusal of stale automatic-enrollment assets and retained enrolling generations.
- Authenticated PK-only removal with unchanged KEK/db, private keys, signed images
  and Secure-Boot target; repeated disable and multiple boots stay in Setup Mode.
- Explicit PK-only re-enrollment and continued signed recovery bootability.
- Refusal of incomplete policy while PK is present, and repair of a missing KEK
  in Setup Mode without rewriting the retained db.

The graph runs CLI argument checks and the key/verification unit contracts first:

```sh
make build unit
nix-shell tests/shell.nix --run 'NIXOS_CONFIG_ROOT=$PWD ./build/nixosctl graph tests/graph.json run secure-boot-removal'
```

This proves the CLI lifecycle against OVMF. It does not establish physical Deck
firmware behavior, firmware-update compatibility, or physical recovery USB
bootability. Independent backup restoration has its own `secure-boot-backup`
scenario.
