# Tests

Scenario specs live under their domain: `installer/`, `immutability/`,
`nixosctl/`, `recovery/`, `secure-boot/`, `tpm2/`, and `qemu/`.
`harness/` tests the runner; its BTRC implementation lives with the CLI under
`config/pkgs/utils/nixosctl/lib/simulator/`.
`core/`, `gaming/` and the `*Tests.btrc` files hold the native unit suites.
BTRC modules use PascalCase filenames matching their primary class.

The graph runner starts each node in a child of its own and runs as many together as fit: `--arg jobs=N`
(by default one for every four processors) and `--arg memoryBudget=48G` (by default three quarters of the
machine's memory, counted from each spec's `memory`). A node starts once every node it comes after has
passed; each running VM is given its own SSH port, so the `sshPort` in a spec is only a default. A failure
stops the nodes that depend on it and nothing else, and the run ends with a summary of what passed, was
skipped, failed or was blocked. With more than one job each node writes `.vm/e2e/logs/<node>.log`.

A node that is one Nix build names it in its spec's `derivation` (what `nix-instantiate` is given) and
builds it with the `build-derivation` operation. The runner makes the `.drv` path the `{{derivation}}`
argument, so the node's state is keyed on exactly what it builds: an edit that does not change the
derivation leaves the node ready. The paths are kept in `.vm/inputs.json` for the tree as it stands (every
file's path, size and time of change), so an unchanged tree is not evaluated again.

`derivation` may also be a flake attribute (`.#packages.x86_64-linux.installer`); `{{derivationOutput}}`
is where its result will be, which is how the installer nodes boot their image straight from the store.
A spec with `"closure": "vm-test"` is keyed on the system its VM-TEST host installs: the runner evaluates
the same host module the guest is given (`{{closure}}`). `inputs` names files and directories a node's
host-side steps read, hashed by content (`{{inputs}}`). A node built on another takes in its parent's
state, so a changed install reruns everything that boots it. The installer images carry this repository,
so any tracked edit rebuilds them and reruns their nodes. Nothing here covers the runner itself: after
changing how nodes are run, use `--arg force=true`.

The graph runner holds `.vm/graph.lock` for a run, so a second run or a collection in the same
workspace is refused. `graph status` ends each line with how long the node's last pass took.
`make graph-gc` deletes the work directories under `.vm/e2e` that no node's current state names,
after printing them; `GC_ARGS=--dry-run` only prints. A directory another state's disk is a delta of
is kept. A spec step written `! command` is refused by the parser when nothing uses its status: write
`if command; then exit 1; fi`. Every node that runs must end in `record-state`.

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
