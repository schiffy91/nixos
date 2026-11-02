# Tests

Scenario specs live under their domain: `installer/`, `immutability/`,
`nixosctl/`, `recovery/`, `secure-boot/`, `tpm2/`, and `qemu/`.
`harness/` tests the runner; `e2e/` contains its BTRC implementation.
`core/`, `gaming/` and the `*Tests.btrc` files hold the native unit suites.
BTRC modules use PascalCase filenames matching their primary class.

Run scenarios through the graph so parents and source hashes are checked:

```sh
make build unit
nix-shell tests/shell.nix --run 'NIXOS_CONFIG_ROOT=$PWD ./build/nixosctl graph tests/graph.json run secure-boot-cli recovery-secure-boot-x86_64 secure-boot-lanzaboote'
make -C tests graph-full
make -C tests gaming
```

`make unit` builds and runs four native suites: immutability persist-key
encoding (`immutability/PathsTests.btrc`), install-root path joining
(`core/NixosPathsTests.btrc`), strict `sbctl`/`bootctl` parsing
(`secure-boot/VerificationTests.btrc`), and the scenario parser
(`harness/SpecTests.btrc`).
`make gaming` (also included in `make check` and `make test`) runs the gaming
package check using the flake's pinned inputs. It builds the real executables
and tests their composed launch path, mod deployment, Steam configuration,
Wine DPI and CM settings. Its entry point is `tests/gaming/default.nix`, which
reuses the production package at `nix/apps/gaming/package.nix` and its check phase.

Secure Boot VM scenarios use the real `nixosctl secure-boot enable` flow with
our own keys, enforcing UEFI firmware, and both x86_64 and aarch64 guests.
Downgrade checks exercise the CLI, direct activation and `NIXOS_NO_CHECK`.
Recovery checks boot the signed image, repair a damaged fallback loader, and
reinstall from recovery. Successful compilation alone proves none of these.

The graph source hash includes BTRC, Nix, scenario data, shell scripts, native
headers and the compiled runner. Do not run individual specs directly or
reuse a ready VM after changing its source without going through the graph.
