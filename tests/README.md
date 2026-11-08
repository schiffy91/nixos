# Tests

Scenario specs live under their domain: `installer/`, `immutability/`,
`nixosctl/`, `recovery/`, `secure-boot/`, `tpm2/`, and `qemu/`.
`harness/` tests the runner; its BTRC implementation lives with the CLI under
`config/pkgs/utils/nixosctl/lib/simulator/`.
`core/`, `gaming/` and the `*Tests.btrc` files hold the native unit suites.
BTRC modules use PascalCase filenames matching their primary class.

Start with focused configuration checks and native suites before VM scenarios:

```sh
make lint
nix build --no-link .#checks.x86_64-linux.host-layout .#checks.x86_64-linux.package-groups
make unit
make gaming
```

`make lint` uses the pinned tools in `ci/tools.nix`. It checks every owned Nix
and BTRC source with the language parser/compiler and the actual language servers,
then runs ShellCheck, JavaScript/Lua parsing, workflow
and manifest checks, native builds/unit suites, and flake evaluation. The native
C check includes Linux helpers, kernel modules and Windows fixtures; its source
inventory must match the repository. Vendored Proton sources and generated build
outputs are outside this inventory. Error diagnostics fail the command; advisory
warnings remain visible in `build/lint/`.

Editor settings live in `.vscode/` and use the pinned BTRC and Nix language
servers. The Nix server has a narrow upstream fix for flake builtins, with a
regression test that still rejects misspelled builtins.
The BTRC compiler and language server share the pinned compatibility fixes in
`config/pkgs/dev/btrc/compat.nix`: libjpeg-turbo's versioned initialization macro
and authenticated stdlib origins for copied native bytes. Editor protocol tests
accept the real `Library.Bytes` and reject a user-defined substitute.

`vm/` builds NixOS nodes with the pinned Nixpkgs test modules, then runs compiled
BTRC scenarios. `Lab` owns guests and virtual networks; `Guest` drives the serial
shell and QMP with bounded commands and framed output. There is no generated
Python test script or Python VM driver. Initrd, reboot and multi-machine tests
use the same implementation. Native Gamescope tests capture QEMU's actual EGL
framebuffer through a typed libvncclient resource, then use BTRC image operations
and Tesseract for observations. Artifacts retain raw PPM images and guest logs.

`lib/` contains checked process results, temporary fixtures, owned children,
PTY sessions, HTTPS observations and independent credential digest checks.
Executable command fixtures are compiled BTRC programs. Native headers expose
bounded TLS transport, CPU affinity, process/signal primitives and framebuffer
bytes; scenario policy and assertions stay in BTRC.

The layout check covers public output names, installer host selection and the
single Mac Home Manager declaration. The app-hierarchy check covers inherited
defaults, explicit parent/child conflicts, nested and dynamically named apps,
and sibling dependencies. The package check verifies real package, service,
persistence and taskbar behavior for valid off configurations, and rejects an
invalid host at system evaluation. Run these with:

```sh
nix build .#checks.x86_64-linux.app-hierarchy .#checks.x86_64-linux.package-groups .#checks.x86_64-linux.persistence-ownership --no-link
```

Run scenarios through the graph so parents and source hashes are checked:

```sh
make build unit
nix-shell tests/shell.nix --run 'NIXOS_CONFIG_ROOT=$PWD ./build/nixosctl graph tests/graph.json run secure-boot-cli recovery-secure-boot-x86_64 secure-boot-lanzaboote'
make -C tests graph-full
make -C tests gaming
```

`make unit` builds and runs native suites including immutability persist-key
encoding (`immutability/PathsTests.btrc`), install-root path joining
(`core/NixosPathsTests.btrc`), strict `sbctl`/`bootctl` parsing
(`secure-boot/VerificationTests.btrc`), and the scenario parser
(`harness/SpecTests.btrc`).
`make gaming` (also included in `make check` and `make test`) runs the gaming
package check using the flake's pinned inputs. It builds the real executables
and tests their composed launch path, mod deployment, Steam configuration,
Wine DPI and CM settings. Its entry point is `tests/gaming/default.nix`; `tests/gaming/tools.nix` exercises
the independently packaged Steam, Wine and Assetto Corsa tools.

Secure Boot VM scenarios use the real `nixosctl secure-boot enable` flow with
our own keys, enforcing UEFI firmware, and both x86_64 and aarch64 guests.
Downgrade checks exercise the CLI, direct activation and `NIXOS_NO_CHECK`.
Recovery checks boot the signed image, repair a damaged fallback loader, and
reinstall from recovery. Successful compilation alone proves none of these.

The graph source hash includes BTRC, Nix, scenario data, shell scripts, native
headers and the compiled runner. Do not run individual specs directly or
reuse a ready VM after changing its source without going through the graph.

`initrd-touch-unlock` runs the BTRC VM driver through a graph host
operation. It reuses the production touch-keyboard module, boots an encrypted
root, delays initrd drivers, rejects a wrong passphrase, and clicks the visible
keyboard to unlock. Its absolute USB tablet input verifies the graphical keys;
it does not emulate the Steam Deck's I2C digitizer or AMD GPU. Run it with:

```sh
nix-shell tests/shell.nix --run 'NIXOS_CONFIG_ROOT=$PWD ./build/nixosctl graph tests/graph.json run initrd-touch-unlock'
```
