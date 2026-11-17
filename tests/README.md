# Tests

Scenario specs live under their domain: `installer/`, `immutability/`,
`nixosctl/`, `recovery/`, `secure-boot/`, `tpm2/`, `boot/`, and `qemu/`.
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
and manifest checks, native builds/unit suites, and flake evaluation. Each file's
compiler runs as its own child, half the processors at a time; the BTRC language
server's files are dealt out to several servers; and the flake outputs are
evaluated four at a time, all but this system's checks, which `ci/checks.sh`
builds. It ends with the time each phase took. The native
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
nix-shell tests/shell.nix --run 'NIXOS_CONFIG_ROOT=$PWD ./build/nixosctl graph tests/graph.json run secure-boot-cli recovery-secure-boot secure-boot-lanzaboote'
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

A scenario that answers the guest's console itself waits with `wait-serial`: it looks only at what the
guest printed after the last text typed there, by `serial-send` or `unlock-luks`, so a prompt that is
back after a wrong answer is told from the one the answer went to. A VM started again begins a new log.
`expect-no-ssh` fails if the guest answers SSH at any point in its timeout: a disk that is still locked,
a generation that did not boot.

What an installed system does when its owner uses it has nodes of its own. `nixosctl-update` runs
`nixosctl update` as the admin user after a change to the host, where every other spec calls
`nixos-rebuild`. `recovery-bad-generation` makes the newest generation fail in its initrd and comes back
by the boot menu and `nixosctl update`. `tpm2-unlock` boots an encrypted install by its TPM2 keyslot,
with nothing typed, and by passphrase again once the slot is wiped. `boot-wrong-passphrase` answers the
boot prompt wrongly twice before the right passphrase and compares the LUKS header with what it was.
`immutability-home` resets `@home` around persisted paths in the admin user's home.

## Ledger

Every node and check that was required on 2026-11-16, and where it is now. A test leaves the VM graph
only for a place that still runs it.

| Was | Is now | Why |
|---|---|---|
| `app-settings` (graph, 44 `nix eval` steps and one shell loop) | flake check `app-settings` | It only evaluates the configuration; one evaluation answers every expectation |
| `qemu-host-x86_64`, `qemu-host-aarch64` | flake check `qemu-hosts`, both architectures | Evaluation needs no builder of the other architecture |
| `opportunistic-unlock-policy`, `secure-boot-key-contracts`, `sleep-lock-state`, `networking-policy`, `recovery-mount-contracts`, `recovery-artifacts` | flake checks of the same names | Each is one derivation; lint built them a second time |
| `host-timeout`, `source-hash`, `graph-disabled`, `graph-runner`, `host-picker` | `make harness` | They test the runner and the picker through their own binaries; a runner is not well tested by a run of itself |
| `spec-toml`, `spec-yaml` | `formats` case of the runner test (`make harness`) | The same two specs, run over a fixture graph |
| `quick` | removed | It printed a word through the runner; every case of the runner test does more |
| `stateful-host` | removed | It proved that a state is recorded and reused; the runner test's duration and collection cases prove the same over fixtures |
| `initrd-touch-unlock-displays` (required) | graph set `alone`, run by `make graph-required` after the required set | It still runs for every commit that changes it, but with nothing beside it: under load a display it plugs in comes up late and Plymouth draws on the firmware framebuffer, and each fix moved the failure to another step |
| `storage` (required) | graph set `alone` | Its card is an emulated SD card. Beside other nodes it failed in two runs out of two after passing in seven: once waiting ninety seconds for the card to mount, once with `mkfs` outlasting a command's ninety seconds |
| `drive-planner`, `drive-remote` | checks `unit-planner`, `unit-remote` of `schiffy91/drive` | The client moved to its own repository; its tests are that flake's checks, keyed on their own derivations |
| `drive-store`, `drive-door`, `drive-readers`, `drive-writes`, `drive-convergence`, `drive-pinning`, `drive-placement`, `drive-crashes`, `drive-held`, `drive-held-placed`, `drive-refusals`, `drive-dialog`, `drive-tray`, `drive-mutations` | checks `vm-<name>` of `schiffy91/drive` (`ci/checks.sh vm` there) | The same scenarios against the same fake Drive; they need KVM, so they run on the owner's machine and are recorded with `ci-local` |
| `drive-convergence-seeds`, graph set `nightly` | `nix build .#nightly.x86_64-linux.convergence-seeds` in `schiffy91/drive` | It was the only member of the set, which is gone with it |
| `drive-module` (a VM that booted the units) | `vm-module` of `schiffy91/drive` boots the units; `drive-module` here is the adapter's test | What is left here is the adapter: the node asserts that a host's settings reach the units and that the client's state is persisted |
| `config/lib/btrc.nix`, `tests/vm`, eight files of `tests/lib` | `btrc.lib.buildPackage`, `btrc.lib.vmTest` and the `testkit` package in btrc | One copy, used by every repository that builds or tests BTRC programs; `tests/lib/vm-test.nix` and `test-source.nix` lay a build out as the checkout is |
| `install-reset-x86_64` | `install-reset`, now in the required set | On this architecture the two were the same install; the twin's disk was never booted |
| `installer-download-x86_64`, `installer-ssh-x86_64` | `installer-download`, `installer-ssh` | A node with no architecture in its name runs on the host's own, so on this desktop each pair was one download and one boot done twice |
| `recovery-boot-x86_64`, `recovery-secure-boot-x86_64` | `recovery-boot`, `recovery-secure-boot`, after `installer-ssh` | The same scenarios on the host's architecture; their parent was the removed twin |
| `installer-ssh-aarch64`, `install-reset-aarch64`, `recovery-boot-aarch64`, `recovery-secure-boot-aarch64`, `secure-boot-aarch64` | graph set `aarch64` (`make -C tests graph-aarch64`) | A run on x86_64 skipped them while listing them as required; the set says where they run |
| `installer-setup`, `installer-up-iso` | removed | Every install node makes the same key and disk and boots the same image through `installer-ssh` |
| `tpm2-probe` | removed | It looked for a TPM device in a booted installer; `tpm2-enroll` enrols with one and `tpm2-unlock` boots by it |
| `initrd-touch-unlock-rotated` | removed | `initrd-touch-unlock-german` runs with the same rotation, plus a layout and the light theme |
| `persistence-ownership` (graph) | flake check `persistence-ownership` | The node built that check; the check is built with the others |
| four `host-hibernate-*` nodes | removed | Neither host hibernates and the nodes were switched off; `immutability-hibernate` still proves a reset never runs before a resume |
| third boot of `host-boot-*` | removed | It ran only for a node that set `reenrollsKeys`, and none did |
| `drive-convergence` over three seeds | one seed required; `drive-convergence-seeds` (`make -C tests graph-nightly`) runs three | Each seed is 120 random operations by two clients; one proves convergence per commit |

Not merged, though the review proposed it: the installs into one chain per host, and `install-reset`'s
children into fewer nodes. With nodes running together a chain is slower than its parts side by side, and
one chain per host would drop the test that installs the Secure Boot target directly.
