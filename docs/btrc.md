# NixOS btrc Rewrite

This folder is the staged rewrite of the Python/Rust management layer in BTRC.
The v2 semipermeable immutability runtime is wired into the live Nix module via
checked-in generated C; the remaining CLI work can be tested from this folder
before replacing the old Python entrypoints.

## Layout

| Path | Purpose |
|---|---|
| `bin/nixosctl.btrc` | CLI entrypoint and command dispatch |
| `src/core/` | Logging and local path/config-file helpers |
| `src/nix/` | Nix eval, rebuild, and secrets wrappers |
| `src/btrfs/` | Snapshot and diff helpers |
| `src/install/` | Disko and `nixos-install` orchestration |
| `src/hardware/` | Secure Boot and TPM2 helpers |
| `src/immutability/` | BTRFS semipermeable membrane runtime |
| `generated/` | Checked-in C generated from BTRC for Nix/initrd builds until the compiler is packaged |
| `PARITY.md` | Side-by-side Python/Rust to BTRC migration ledger |
| `tests/e2e/` | JSON-driven QEMU VM/state-chain runner implementation |
| `tests/` | Nix shell, test Makefile, and per-test spec directories |
| `vendor/btrc-stdlib/` | Source copy of the btrc standard library used by this rewrite |

## Build

The NixOS flake depends on the BTRC flake (`github:schiffy91/btrc`) through `inputs.btrc`.
The Makefile uses `nix run ..#btrc` for transpilation and
`nix develop ..#btrc-build` for C compilation, so builds flow through the flake
dependency graph instead of shelling into `../../btrc`.

```bash
cd btrc
make build
make generated
make stdlib-sync-check
make quick
make app-settings
make stateful-host
make aarch64-qemu-host
make chain
make graph-status
make graph-coverage
make graph-full
```

`make smoke` remains as a legacy alias for `make quick`, but the E2E harness is
the JSON-driven runner below.
`make stdlib-sync-check` compares `vendor/btrc-stdlib` with the canonical
compiler stdlib exposed by `..#btrcSrc` so the vendored copy cannot drift
silently.
`make chain` is a real QEMU chain now: it boots the installer, installs NixOS,
boots the installed disk, and runs the reset-mode immutability check.
`make graph-full` is the preferred scaffolded E2E path: it runs the declared
graph in `tests/graph.json`, skips ready ancestors by hash, and branches VM
states using qcow2 backing deltas.

## CLI Shape

```bash
NIXOS_CONFIG_ROOT=/etc/nixos build/nixosctl eval config.settings.disk.device
sudo NIXOS_CONFIG_ROOT=/etc/nixos build/nixosctl update --upgrade
sudo NIXOS_CONFIG_ROOT=/etc/nixos build/nixosctl upgrade
sudo NIXOS_CONFIG_ROOT=/etc/nixos build/nixosctl install --format
sudo NIXOS_CONFIG_ROOT=/etc/nixos build/nixosctl diff --pattern '/var/*'
sudo NIXOS_CONFIG_ROOT=/etc/nixos build/nixosctl fix-permissions
sudo NIXOS_CONFIG_ROOT=/etc/nixos build/nixosctl change-password --old current --new replacement
sudo NIXOS_CONFIG_ROOT=/etc/nixos build/nixosctl secure-boot enable
sudo NIXOS_CONFIG_ROOT=/etc/nixos build/nixosctl tpm2 status
sudo build/semipermeable_membrane --dry-run /dev/root @snapshots CLEAN disabled @persist /etc/semipermeable_membrane/spec.tsv
build/nixosctl e2e tests/quick/test.json
build/nixosctl e2e tests/stateful-host/test.json --arg marker=my-run
build/nixosctl e2e tests/installer-download/test.json
build/nixosctl e2e tests/install-system/test.json
build/nixosctl e2e tests/immutability-reset/test.json
build/nixosctl e2e tests/installer-ssh/test.json
build/nixosctl vm tests/installer-ssh/test.json status
build/nixosctl vm tests/installer-ssh/test.json ssh 'uname -a'
build/nixosctl graph tests/graph.json status
build/nixosctl graph tests/graph.json run app-settings
build/nixosctl graph tests/graph.json run immutability-reset install-disabled
```

`UnixShell` is the implemented shell backend. `PowerShell` remains a stdlib TODO
so the command API can become cross-platform without pretending Windows support
already exists.

## Known Gaps

The BTRC layer now owns the CLI, VM harness, snapshots/diff, install/update,
desktop helper commands, and the semipermeable immutability runtime. Remaining
gaps are intentionally scoped to missing BTRC platform/library surfaces.

| Area | Remaining work |
|---|---|
| desktop tray daemon | Declarative `Tray`, `Window`, `HtmlView`, `NativeUiBackend`, `DaemonApp`, and daemon control primitives exist; the remaining work is a persistent tray/WebView event loop backend |
| hardware-only paths | TPM2 is covered with `swtpm` in QEMU, and Secure Boot/Lanzaboote has an x86_64 QEMU capability probe plus an optional provisioning graph; real hardware enrollment still needs final firmware validation |

## Stdlib Direction

The native GUI path for BTRC is a small declarative app layer with native
process-backed primitives first and persistent WebView/native widget backends
behind the same interface later. This stays lightweight: simple tools can use
`open`/`osascript` on macOS and `xdg-open`/`notify-send`/`zenity` on Linux,
while richer apps can later bind WKWebView, WebKitGTK, or WinUI/WebView2
without changing application models.

The BTRC compiler and stdlib are consumed through this repo's flake dependency
graph as `inputs.btrc`, pinned in `flake.lock` to `github:schiffy91/btrc`.
This repository vendors only `src/stdlib` because the NixOS management programs
compile with `--no-stdlib` and explicit include ordering. A full git submodule
would bring in the compiler, tests, editor tooling, and generated artifacts into
the NixOS repo; the KISS boundary is a flake input plus the
`stdlib-sync-check` target, which diffs the vendored copy against `..#btrcSrc`.
The Makefile contract (`nix run ..#btrc`, `nix develop ..#btrc-build`) is
unchanged by the source URL, so local development against the sibling
`~/Drive/dev/btrc` checkout still works after `nix flake update btrc`.

| Surface | Shape |
|---|---|
| `UiNode` / `UiDocument` | Declarative tree builder that renders escaped local HTML and CSS; `Ui.rawHtml` exists for trusted HTML fragments |
| `HtmlView` / `Window` | File-backed HTML view model plus platform opener backends (`LinuxUiBuilder`, `MacUiBuilder`, `WindowsUiBuilder`) |
| `NativeUiBackend` | KISS native command bridge for notifications, dialogs, and opening HTML/files; Linux and macOS are implemented, Windows remains TODO |
| `State<T>` / `Signal` | Generic state cells plus simple string event streams; worker code can use the language's existing `spawn`, `Thread<T>`, and `Mutex<T>` primitives |
| `DaemonSpec` / `DaemonController` / `DaemonApp` | Declarative daemon intent and simple Unix start/stop/status supervision using `Command` and `UnixShell` |
| `NativeView` / `Tray` | Declarative native/tray models; GTK/libadwaita, AppKit, WinUI, and WebView presentation backends are the next platform bindings |

The compiler now supports explicit package imports plus controlled bulk imports:

```btrc
import std.{cli, fs, json, process, toml, ui}
import ./src/immutability/**
import ./src/desktop/*
```

Supported forms are `std.name`, `std.{a, b}`, `std.*`, relative files,
directory `*`, and recursive directory `**`. `bin/nixosctl.btrc` still keeps
explicit stdlib ordering for now because the vendored stdlib is intentionally
compiled with `--no-stdlib`.

## E2E VM Specs

VM E2E tests are JSON, TOML, or strict YAML files consumed by
`nixosctl e2e <spec>`.
Each test lives in its own subdirectory under `tests/`; shared VM artifacts and
downloaded installer ISOs live under `.vm/`, which is ignored by git.

Use the test Makefile through `nix-shell`:

```bash
make -C btrc/tests quick
make -C btrc/tests app-settings
make -C btrc/tests stateful-host
make -C btrc/tests aarch64-qemu-host
make -C btrc/tests installer-download
make -C btrc/tests install-system
make -C btrc/tests immutability-reset
make -C btrc/tests installer-ssh
make -C btrc/tests tpm2-probe
make -C btrc/tests secure-boot-capabilities
make -C btrc/tests secure-boot-lanzaboote
make -C btrc/tests chain
make -C btrc/tests graph-status
make -C btrc/tests graph-full
```

`quick`, `stateful-host`, and `installer-download` are host-only or cheap.
`installer-ssh` boots QEMU. `install-system`, `immutability-reset`, and `chain`
perform a real NixOS install inside the VM.
`graph-full` runs the graph declared in `tests/graph.json`; it is the most
useful target for repeated E2E work because already-ready ancestors are skipped
unless `--arg force=true` is supplied.

`tests/shell.nix` provides QEMU, OpenSSH, `qemu-img`, `curl`, `bsdtar`, `socat`,
and GNU `timeout`. The default ISO URL follows the host architecture and uses
`https://channels.nixos.org/nixos-unstable/latest-nixos-minimal-<arch>-linux.iso`.

The runner currently supports:

| Operation | Effect |
|---|---|
| `download-iso` | Download the configured installer ISO if missing |
| `require-command` | Require a host command to exist |
| `require-tpm2` | Require `swtpm` plus the matching QEMU TPM device |
| `require-uefi` | Require QEMU UEFI firmware for the spec architecture |
| `require-secure-boot` | Require x86_64 Secure Boot firmware, `swtpm`, and TPM QEMU support |
| `probe-secure-boot` | Print the Secure Boot QEMU capability report without starting a VM |
| `nix-eval` | Evaluate a Nix expression with normal `nix eval` output so specs can assert booleans, strings, and paths |
| `require-state` | Require the parent state hash to exist before running |
| `inherit-state` | Reconstruct this state from its parent; VM disks use qcow2 backing overlays when a parent disk exists |
| `record-state` | Write the state hash, work directory, and metadata under the chain root |
| `reset-state` | Stop the VM and remove the per-test `workDir` for a reproducible clean run |
| `validate-immutability-v2` | Fail if a btrc spec tries to use immutability v1 |
| `create-key` | Create the per-test SSH key |
| `create-disk` | Create the per-test qcow2 disk |
| `boot-iso` / `boot-disk` | Start QEMU as a daemon with user-networked SSH forwarding |
| `bootstrap-ssh` | Send root SSH key setup commands through the QEMU serial pipe |
| `wait-ssh` | Poll SSH until the guest is ready |
| `guest` / `host` | Run a command and assert optional `expect` text |
| `qmp` | Send a JSON QMP command over the per-VM QMP socket and assert optional `expect` text |
| `copy-to` / `copy-from` | Copy files over SCP |
| `snapshot` / `restore` | Manage qcow2 snapshots |
| `copy-workspace` | Stream the local repo into the guest over SSH, excluding `.git`, secrets, and VM artifacts |
| `configure-vm-host` | Write the minimal `VM-TEST.nix` and `config.json` inside the guest from spec args |
| `install-nixos` | Run disko, `nixos-install`, and initial clean BTRFS snapshot creation inside the live installer |
| `reboot-disk` | Reboot the installed guest, restart QEMU from disk, and wait for SSH |
| `sleep` / `serial-send` / `stop` | Timing, raw serial command, and shutdown helpers |

Specs can define string args and refer to them with `{{name}}` or `${name}` in
string fields. CLI overrides use `--arg key=value`:

```json
{
  "name": "stateful-host",
  "state": "stateful-host",
  "parentState": "quick",
  "stateMaterial": "host-state-marker:{{marker}}",
  "args": {
    "marker": "default"
  },
  "workDir": ".vm/e2e/{{state}}-{{stateHashShort}}",
  "operations": [
    { "op": "require-state" },
    { "op": "reset-state" },
    { "op": "host", "command": "printf {{marker}} > {{workDir}}/state.txt" },
    { "op": "host", "command": "cat {{workDir}}/state.txt", "expect": "{{marker}}" },
    { "op": "record-state" }
  ]
}
```

The runner also exposes derived args: `name`, `workDir`, `arch`, `iso`,
`isoUrl`, `diskSize`, `memory`, `cpus`, `sshPort`, `state`, `parentState`,
`parentHash`, `stateHash`, `stateHashShort`, and graph-provided `sourceHash`.
`sourceHash` is a digest of the workspace source plus the built
`btrc/build/nixosctl` binary. This makes state paths, snapshot names, host
commands, and guest commands deterministic from the spec plus explicit args.

## E2E State Chain

Specs form a hash-addressed tree. Each state hash is computed from:

```text
parent=<parentHash>
state=<state>
material=<stateMaterial>
operations=<expanded operations>
```

Recorded states live under `.vm/e2e/chain/<state>/`.

`tests/graph.json` is the literal graph. Each node points at a spec file,
declares `after` dependencies, and can override spec args. The graph runner
walks dependencies first, detects cycles, skips ready nodes whose hash matches
the recorded state, and creates child disks as qcow2 overlays backed by the
parent disk. That gives us reproducible state reconstruction without rerunning
the installer for every permutation.

| Graph node | State | Parent | Purpose |
|---|---|---|---|
| `quick` | `quick` | `root` | Cheap harness sanity check |
| `stateful-host` | `stateful-host` | `quick` | Host state/hash check |
| `app-settings` | `app-settings` | `quick` | Positive Nix eval checks for app settings defaults and host-level Steam/Sunshine enablement |
| `aarch64-qemu-host` | `aarch64-QEMU-host` | `quick` | Confirms the uppercase `QEMU` aarch64 host evaluates, uses `/dev/vda`, and builds a full toplevel |
| `cli-parity` | `cli-parity` | `quick` | Host-side parity checks for audio listing, secure boot status parsing, and permissions |
| `spec-toml` | `spec-toml` | `quick` | TOML e2e spec parser coverage |
| `spec-yaml` | `spec-yaml` | `spec-toml` | YAML e2e spec parser coverage |
| `installer-download` | `base-iso` | `root` | Download and verify the NixOS minimal installer ISO |
| `installer-setup` | `installer-setup` | `base-iso` | Runs the atomic setup operation and validates ISO, disk, and SSH key material |
| `installer-up-iso` | `installer-up-iso` | `installer-setup` | Runs the composed up-iso operation and validates the live installer |
| `installer-ssh` | `installer-ssh` | `base-iso` | Boot the minimal ISO, inject root SSH through serial, assert installer state, and snapshot |
| `installer-download-x86_64` | `base-iso-x86_64` | `root` | Download the x86_64 installer ISO for Secure Boot-capable QEMU hosts |
| `installer-ssh-x86_64` | `installer-ssh-x86_64` | `base-iso-x86_64` | x86_64 live installer parent for Secure Boot provisioning tests |
| `qmp-probe` | `qmp-probe` | `base-iso` | Boot QEMU and query `query-status` over QMP |
| `tpm2-probe` | `tpm2-probe` | `base-iso` | Boot QEMU with `swtpm`, verify TPM2 in `/sys/class/tpm/tpm0`, and query QMP |
| `secure-boot-capabilities` | `secure-boot-capabilities` | `quick` | Report x86_64 QEMU Secure Boot, EDK2, TPM, and `swtpm` support |
| `install-reset` | `installed-reset` | `installer-ssh` | Copy this repo, configure `VM-TEST`, run disko plus `nixos-install` with semipermeable reset mode, create clean BTRFS snapshots, and snapshot the installed disk |
| `immutability-reset` | `immutability-reset` | `installed-reset` | Boot the installed disk, write persistent and ephemeral markers, reboot, and verify reset behavior |
| `install-disabled` | `installed-disabled` | `installer-ssh` | Install the disabled-immutability permutation from the same live installer parent |
| `secure-boot-install` | `installed-secure-boot` | `installer-ssh-x86_64` | Install the Secure-Boot target with Lanzaboote auto-generate/auto-enroll enabled and TPM2 attached |
| `secure-boot-lanzaboote` | `secure-boot-lanzaboote` | `installed-secure-boot` | Optional x86_64 Secure Boot provisioning check: first boot writes auth variables/signs ESP, reboot verifies enabled Secure Boot |

Immutability v1 remains only in the original Rust initrd implementation under
the live NixOS modules. The btrc E2E chain rejects v1 and treats v2 as the only
btrc-side immutability contract.

`nixosctl vm <spec.json> ...` is the manual front door to the same framework;
there is no separate VM implementation. Useful commands:

```bash
build/nixosctl vm tests/installer-ssh/test.json status
build/nixosctl vm tests/installer-ssh/test.json up
build/nixosctl vm tests/installer-ssh/test.json ssh 'hostname'
build/nixosctl vm tests/installer-ssh/test.json snapshot --name live-ssh
build/nixosctl vm tests/installer-ssh/test.json stop
```

For x86_64 installer ISOs, `boot-iso` extracts the ISO's `boot-serial` kernel
and initrd via `bsdtar`, then boots them directly so serial bootstrap is
scriptable. This keeps the harness btrc-owned while still using QEMU, SSH, and
NixOS as the actual system under test.

For aarch64 installer ISOs, `boot-iso` uses QEMU's packaged EDK2 firmware with a
per-test writable vars file, boots the minimal ISO, drains serial output to
`serial.log`, and injects the SSH key through the auto-logged-in `nixos` serial
session. QEMU user networking forwards `localhost:<sshPort>` to the guest's SSH
port, so tests never need bridged networking or root-only host networking.

TPM2 tests use `swtpm socket` and attach it to QEMU with `-tpmdev emulator`.
The TPM state directory is part of each test work directory and is copied into
child graph nodes alongside qcow2 deltas and EDK2 vars. Secure Boot enforcement
uses x86_64 secure EDK2 firmware when available; the full Lanzaboote graph is
intentionally opt-in because cross-architecture x86_64 TCG can be slow on
Apple Silicon.

The approach is grounded in the NixOS manual's installer SSH note and QEMU's
documented direct-kernel, serial pipe, host port forwarding, daemon, and pidfile
options:

| Topic | Source |
|---|---|
| NixOS installer SSH key locations | https://nixos.org/manual/nixos/stable/#sec-installation-manual-networking |
| QEMU direct kernel boot | https://www.qemu.org/docs/master/system/linuxboot.html |
| QEMU host port forwarding, serial pipe, daemon, pidfile | https://www.qemu.org/docs/master/system/invocation.html |
