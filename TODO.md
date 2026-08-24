# TODO

## Precise dependency hashing for e2e graph states

`tests/e2e/graph/runner.btrc` keys every graph node's state on a single
`sourceHash` — sha256 over every tracked source file (`*.btrc`/`*.nix`/`*.json`/etc.)
plus the built `nixosctl` binary. It's folded into every node's hash material
regardless of whether that node's own `stateMaterial` template references it
(`VmTestSpec.hashMaterial()` in `tests/e2e/spec/test_spec.btrc` includes the
full sorted args map, and `sourceHash` is always one of those args).

Effect: any edit to any tracked file anywhere in the repo invalidates the
*entire* chain, including `installer-download`/`installer-ssh`, which only
fetch a public NixOS ISO and build nothing from this repo.

Fix: use what Nix already computes — the resolved store path of
`config.system.build.toplevel` (or whichever derivation a scenario actually
depends on) as that node's material, instead of a blanket file-tree hash.
`install-*` nodes would then only redo when their real build closure changes;
download/probe nodes wouldn't be touched by btrc edits at all.

Tradeoff: trades one cheap `sha256` pass over the tree for a `nix eval` per
node (slower per check, cacheable within a run). Scenario-level nodes
(`password-change`, `tpm2-enroll`) exercise the `nixosctl`/`immutability`
binaries at runtime, not just the installed config, so their material needs
the toplevel hash *plus* the binary hash (the runner already appends
`:nixosctl=<hash>`, redundantly on top of the blanket hash today).
