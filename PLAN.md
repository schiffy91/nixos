# Faster tests, then products out of this repository

Written 2026-11-16 for review. Nothing in it has been done yet. It joins two plans: the test-suite review
(Part 1) and the split into product repositories (Part 2). When the work is finished the lasting facts move
into each repository's README and this file is deleted.

## Why one plan, and why in this order

Part 2 leans on the tests at every step, and today they are slow and in places cannot fail. The required
graph takes about 3 h 20 min after any source change, because every node keys on a hash of the whole
repository; a lock bump for one product would make all of them stale. Nine assertions in the immutability
specs pass whatever happens, and Part 2 changes how immutability resets. So the tests are made trustworthy
and fast first, the missing scenarios are written while the code is still in one place, and then the
products move, each carrying its tests.

| Order | Stage | What it gives the stages after it |
|---|---|---|
| 1 | T0. Trust and measurement | Passes mean something; every later step has timings |
| 2 | T1. NixOS speed | A source change reruns only what it touches; the graph runs in parallel |
| 3 | T2. btrc speed | btrc's own gates are short enough to run at every step |
| 4 | T3. Coverage | Drive, reset and unlock scenarios exist before the code that they cover moves or changes |
| 5 | T4. Delete | Less to move |
| 6 | M1 to M9. The split | |
| 7 | Finish | Measured targets and one full run on the final tree |

## The end state

This repository goes back to being a NixOS configuration: hosts, the system layer, `nixosctl`, and one
short adapter per product. Each product that is useful on a machine that is not ours lives in its own
repository with its own flake, tests and CI, and is consumed here as a flake input, the way Semu already is.

| Piece | Today | After |
|---|---|---|
| Required graph, after a source change | about 3 h 20 min, serial | under 35 min, parallel |
| Required graph, nothing changed | about 39 min | under 3 min |
| `make lint` | 21 min | under 8 min |
| `make unit` after a one-file edit | rebuilds everything | under 30 s |
| btrc inner loop | none | `make test-fast`, under 5 min |
| btrc `make test` on this desktop | 30 to 53 min recorded on the Mac | under 30 min |
| BTRC package builder | `config/lib/btrc.nix`, imported by 16 files under `config/` and by the tests | `btrc.lib.buildPackage` |
| BTRC VM test library | `tests/vm`, `tests/lib` | `btrc.lib.vmTest` and a library package in btrc |
| `ci-local` | `config/pkgs/dev/ci-local`, the `local` job copied into every workflow | `schiffy91/ci-local`, one reusable workflow |
| Drive | `config/pkgs/utils/drive`, `tests/drive` | `schiffy91/drive` |
| Deck Pad | `config/pkgs/gaming/deckpad`, `tests/gaming/deckpad` | `schiffy91/deckpad` |
| Proton SGE | `config/pkgs/gaming/proton/custom` | `schiffy91/proton-sge` |
| WoW addons | patches chosen by addon name in `addons.nix` | patches are data; still here |
| System tray | fixed menu plus static entries | entries can show a line of status and ask for attention; still in `nixosctl` |
| Immutability | resets to a snapshot taken after install | resets to an empty root; then `schiffy91/immutability` |

The "today" timings come from the review's receipts, CI history and state-directory timestamps. T0 measures
them again before anything changes.

## Rules for the whole plan

1. **Nothing of the owner's is deleted, formatted or overwritten.** That covers the Drive library, game
   saves, the SD cards and the `CLEAN` snapshots on the real machines. The one deletion this plan makes is
   `nixosctl graph gc` inside `.vm/e2e` (T0.6), which prints its list first.
2. **No emulator or game is launched against a real library.** The last test pass that did so deleted
   saves. Battle.net's launcher window is the one exception (M5); no game is started from it.
3. **Tests use the fake Drive.** Nothing is created in the real Drive.
4. **Scenarios run only through `nixosctl graph tests/graph.json run <node>`.**
5. **The Deck is not rebooted or powered off.** Its screen stays black (console log level 1, an empty
   console, panel off, sleep inhibited, sound muted). Before a switch on the Deck, check that
   `deckpad.service` is not serving and that no `reaper SteamLaunch` process exists. Neither `drive resume`
   nor `drive keep` is run on either machine.
6. **The Deck never compiles anything large.** Publish to the LAN cache from the desktop first (`make cache`).
7. **Part 1 switches no machine.** In Part 2: one system switch at a time, never during a graph run, and
   never two graph invocations at once. The desktop is switched from this checkout with
   `sudo nixosctl update`, never from a clean worktree.
8. **Build every flake check before committing config.** Each numbered step is its own commit, with the
   tests that prove it. Commits are signed with the 1Password key, pushed with `ci-local push`, never
   forced, and no history is rewritten in an existing repository.
9. **Fetch before every commit and build on `origin/main`.** The other agent that pushed to btrc and nixos
   was stopped on 16 November, but its work may still be in flight: before moving a directory, look for
   work that touches it (other clones under `~/Projects`, open branches). If there is any, do a different
   step and list this one as waiting.
10. **New repositories are public** (their code is already public here) and are created with
    `gh repo create schiffy91/<name> --public`. Before a repository's first push, scan everything going into
    it for credentials and for host or user names; a product repository carries neither.
11. **History travels with the code.** Each new repository starts from a filtered copy of this one's history
    for the moved paths, made in a scratch clone. This repository's history is untouched.
12. **No litter.** Scratch clones, logs and worktrees live in the session's scratch folder and are removed.
13. **No test is dropped silently.** `tests/README.md` gains a ledger: every node and check that is required
    today is still required, merged into a named node, moved to a named repository, or deleted with the
    reason. Later stages name tests through this ledger.
14. **If a step needs the owner** (a reboot, Mac `sudo`, a locked 1Password, someone at the Deck), do
    everything else and put it in "Waiting for the owner" below. A waiting item is not a failure and is not
    worked around.
15. **Failures are reported as failures**, with the output.
16. **Test jobs run inside an isolated process namespace** (`bwrap --unshare-pid`; `make -C tests` does it
    where bwrap exists), so nothing a test signals can reach the desktop session, and nothing of a run is
    left when it ends. On 16 November a btrc test that kills its parent to play a crashed worker ran after
    that parent had gone; its parent was then the session's own manager, and every process of the desktop
    session was killed. A test that signals its parent first checks that the parent is what it started.
17. **Parallel jobs are sized to the machine on purpose** (32 threads, 62 GB): say how many VMs and how many
    test workers before starting two jobs together, and never stack a third heavy job on two.
18. **About every 30 minutes of work the owner gets an executive summary:** bottom line first, a scorecard
    of before, now and target for each measure, what moved, risks, what is next, and what needs the owner;
    plain words, no step names from this plan.
19. **The work is linear, and subagents have four uses only.** Read-only searches. The btrc track (T2,
    T3.4) running beside the NixOS track (T1). Writing T3's independent scenarios, each in its own worktree
    that is removed afterwards. An independent review of each stage's diff before it is pushed. The main
    session does everything else: every graph run, every timing (measured with nothing else running), every
    push, every system switch, every repository creation, and anything on the Deck or the real Drive.

Where this differs from the test-suite review as it was written:

- **It lands on `main` in small pushed commits**, not on an unpushed `test-speed` branch. Part 2 has to be
  pushed for the machines and the new repositories to consume it, and a long unpushed branch would collide
  with the other agent's merges.
- **`immutability-hibernate` stays in the required set.** The owner has said to keep testing it in the VMs.
  With the parallel runner its five minutes cost nothing on the wall clock.
- **"Nightly" is a named set, not a schedule.** Nothing runs on a timer today. The graph gains a `nightly`
  set and btrc a `make test-nightly`; both are run once by hand at Finish. Scheduling them waits for the
  CI machine.
- **The Deck's held deletions are no longer waiting.** They were put back on 16 November with `drive keep`,
  which has a scenario. T3.1 covers the cases that scenario does not.

# Part 1. Tests

## T0. Trust and measurement

1. `ci/checks.sh` fails on an empty or failed check list, and one `nix` invocation builds all checks.
   Proof: with the listing made to fail, the script exits non-zero.
2. Fix the nine assertions that cannot fail: mostly `! cmd` under `set -e` in the immutability specs
   (`generations`, `restore`, `materialize`, `key-encoding`), and `tpm2-enroll`, which passes even if
   `tpm2 status` crashes. For each, first show the fixed assertion failing on a broken condition.
3. Add `record-state` to the six specs that lack it, so nine nodes stop rerunning on every invocation.
4. `graph status` shows a duration per node; btrc's pytest runs take `--durations` and write JUnit output.
5. `.btrc-cache` and `__pycache__` are left out of the source hash.
6. `nixosctl graph gc` prints the `.vm/e2e` state directories that no current node hash references, then
   deletes them. Today there are 670 directories there, about 750 GB as `du` reports it.

Also in T0, the baseline everything later is compared with, saved in the session's evidence folder:

- `make lint`, `ci/checks.sh`, `make unit` and `make graph-required` timed on `origin/main`; btrc's
  `make test` timed here.
- The built system path of each host, and the generated unit text for Drive and Deck Pad on both Linux hosts.
- On each Linux machine: the Drive status line, the count of files in the library, and `drive held`.
- Rule 9's look for work in flight on every path this plan moves.

Done when the six steps are committed with their proofs, the baseline is on disk, and this plan is committed.

## T1. NixOS speed

1. **Parallel graph runner:** a job pool with a memory budget, an allocated SSH port per VM (15 are shared
   today), continue past failures, and a summary at the end.
2. **Per-node inputs instead of the whole-repository hash:** a Nix-backed node keys on its derivation, an
   install node on the system closure it installs. Proof: an edit to one product reruns only its nodes, and
   a lock bump reruns only the nodes whose derivation or closure changed.
3. **From-scratch x86_64 installs go from 13 to about 7**, served from the local binary cache
   (`make cache-serve`) instead of about 1.7 GiB each from cache.nixos.org.
4. **`install-reset`'s 14 children merge into about 6**, with no assertion lost (the ledger lists each).
5. **`hosts-boot` polls instead of sleeping** (166 s of each 215 to 296 s run is sleep) and drops the third
   boot that does nothing. Not done, on purpose: one install chain per host, and step 4's merge. Both were
   written for a runner that ran one node at a time; with nodes running together a chain is slower than
   its parts side by side, and the chain would drop the test of installing the Secure Boot target
   directly. The ledger records it.
6. **`drive-mutations` fails fast** instead of waiting out three 90 s timeouts; `drive-convergence` runs one
   seed in the required set and three in `nightly`.
7. **The 16 eval and unit nodes leave the graph** for flake checks and `make unit`.
8. **Makefile:** per-program dependencies, the compiler resolved once, `-j` by default. Tighter Nix
   filesets, so a tray edit does not rerun every Drive node.
9. **Lint:** one compiler pass per `.btrc` file, and no flake evaluation that `ci/checks.sh` repeats.
10. **aarch64 VM nodes leave the x86_64 required set** for a set named `aarch64`; the aarch64 eval check
    stays per commit. The Mac runs that set natively if the graph runner works there; if it does not, the
    desktop runs it under emulation once at Finish.

Done when, measured on this desktop: the x86_64 required graph takes under 35 min after a source change and
under 3 min with nothing changed, `make lint` under 8 min, and `make unit` after a one-file edit under 30 s.

## T2. btrc speed

1. `make test-fast`: in-process tests, LSP, formatter, and one reference-compiler corpus pass.
2. The four 600-second load tests move to `make test-nightly`; the 10-second variant stays in `make test`.
3. One shared compiler cache. Helper drivers are built by `btrcc`; a small sample is still built by
   `btrcpy`.
4. The cache key covers only the stdlib modules the compiler imports.
5. CI builds the compiler and the container once per run and shares them; `ci-local` reuses a compiler whose
   fingerprint matches.
6. The corpus is transpiled once per compiler and the C reused across the C11 cells. O1, O3 and report-only
   Wayland move to nightly. Fewer GUI partitions.
7. An expected-skip manifest for native NixOS, so `make test` passes its skip gate here.

Done when, measured here: `make test-fast` under 5 min and `make test` under 30 min.

## T3. Coverage

Written before the code they cover moves or changes.

1. **Drive** (before M3): held deletions across an engine restart and across a reboot; `keep` and `resume`
   on a placed folder; out of space; a revoked sign-in seen by the engine.
2. **NixOS** (before M8): `nixosctl update` succeeding, where every spec today calls `nixos-rebuild`
   directly; recovery from a bad generation; TPM2 unlocking after a reboot; a wrong LUKS passphrase at
   boot; `@home` being reset; the two real hosts' closures building, not only evaluating.
3. **Gaming Mode sleep-lock nodes:** fix the two that fail and make all five runnable on the Deck, which
   has the AMD render node the desktop lacks. Running them there puts the Deck to sleep, so that run is on
   the waiting list and nothing is run on the Deck for this step.
4. **btrc:** line coverage in one job; tests for Realtime, USB, Bluetooth, Crypto, Daemon and Terminal.

Each new scenario is shown failing once, with the behaviour it guards taken away.

## T4. Delete

Each with its reason in the ledger:

- The duplicate `-x86_64` twins (`install-reset-x86_64` is identical to `install-reset` and never booted).
- `stateful-host`, `installer-setup`, `installer-up-iso`, `tpm2-probe`, `initrd-touch-unlock-rotated`.
- The `persistence-ownership` graph node; the flake check of the same name stays.
- The four disabled host-hibernate nodes. `immutability-hibernate` is not one of them and stays.
- `tests/gaming/deckpad/kernel.nix` stays until M4: Deck Pad's README documents it as the by-hand test of the
  kernel's side of each link, so M4 either makes it a check of the new repository or removes it there.
- Stale `__pycache__` directories and `tests/nixd` are not in git. They are files in the owner's checkout,
  which rule 1 keeps this plan from deleting; they are on the waiting list.

Done when the ledger accounts for every node and check that was required at T0, and the required graph
passes in one run.

# Part 2. The split

## The shape of a product repository

```
flake.nix          inputs: nixpkgs, btrc. outputs: packages, nixosModules.default, checks, devShells
src/               the BTRC package (btrc.toml, btrc.lock, sources)
nix/package.nix    built with btrc.lib.buildPackage
nix/module.nix     options under the product's own name, e.g. services.drive
tests/             unit suites and VM scenarios (btrc.lib.vmTest)
ci/local.json      the jobs a local pass can stand in for
ci/checks.sh       builds every flake check, failing on an empty list
.github/workflows/checks.yml   calls the ci-local reusable workflow
README.md
```

The module knows nothing about this configuration: no `settings.*`, no user names, no persistence, no tray,
no firewall abstraction. What it needs is an option; what the host needs to know comes back as a read-only
option (`statePaths`, the port it listens on). The adapter here connects the two. For Drive it is the whole
of `config/pkgs/utils/drive/default.nix`:

```nix
{ config, inputs, lib, ... }: let settings = config.settings.apps.utils.drive; in {
  imports = [ inputs.drive.nixosModules.default ];
  services.drive = { inherit (settings) enable mountPoint store pinned ignore; user = config.settings.users.admin.username; };
  settings.disk.immutability.persist.paths = config.services.drive.statePaths;
}
```

`settings.*` stays the only thing a host file sets, so no host file changes.

Flake inputs follow one pattern:

```nix
drive = { url = "github:schiffy91/drive"; inputs.nixpkgs.follows = "nixpkgs-unstable"; inputs.btrc.follows = "btrc"; };
```

Two checks prove that a move changed nothing:

- **Units:** the systemd units a product generates for FRACTAL-NORTH and STEAM-DECK-OLED are the same text
  before and after, with store hashes masked.
- **Closure:** `nix store diff-closures` between the T0 baseline system and the new one shows no package
  added, removed or changed in version, apart from the renames this plan makes.

A product's scenarios leave the graph and become flake checks in its repository, keyed on their own
derivations as T1.2 made them here. What stays in the graph is the test of the adapter.

Each milestone ends with its own commits pushed and its checks green. Later milestones do not start on a
red tree.

## M1. btrc exports the builder and the VM test library

In `schiffy91/btrc`:

- `lib.buildPackage pkgs { name; src; programs; buildInputs; runtimeInputs; ... }`, the body of today's
  `config/lib/btrc.nix` with the same arguments.
- `lib.vmTest pkgs { name; nodes; scenario; extraFiles; settings; graphical; }`, the body of today's
  `tests/vm/test.nix`. `Lab`, `Guest`, `Channel`, `Framebuffer`, `Keyboard`, `vnc.h` and the `tests/lib`
  files they and the product tests import (`Check`, `TestDirectory`, `CommandLog`, `OwnedProcess`, …) move
  with it as one library that a scenario imports by a stable path.
- Two new flake checks: a program with a `pkg-config` dependency built by `buildPackage`, and one VM
  scenario that boots a node, runs a command, reboots and reads a file back. The VM check needs KVM, so it
  is a local job in `ci/tiers.toml`.

Here:

- Every importer of `config/lib/btrc.nix` (16 files under `config/`, `config/pkgs/gaming/lib/build-tools.nix`
  included, plus the tests) and every test that imports `tests/vm/test.nix` uses the btrc functions.
  `config/lib/btrc.nix`, `tests/vm` and the moved `tests/lib` files are deleted; one copy exists.
- `flake.lock` takes the new btrc.

Verify: btrc's two new checks pass, and `make test-fast` and `make test` still pass. Here, `make lint` and
`ci/checks.sh` pass, both hosts build, the closure check passes, and the Drive and storage nodes pass.

## M2. `schiffy91/ci-local`

- The repository holds `ci-local.py`, a flake with `packages.default`, the README section now in this
  repository, and `tests/checks/ci_local.nix` as its own check.
- A reusable workflow, `.github/workflows/local-passes.yml` (`workflow_call`, output `passed`), replaces
  the `local` job copied into nixos, semu and btrc. Callers pin it by commit.
- Here, `config/pkgs/dev/ci-local` becomes the input's package (desktop, Deck, mbp21). btrc keeps
  generating its job table from `ci/tiers.toml`, with the compiler reuse from T2.5.

Verify, in each of the three repositories: a commit pushed with `ci-local push` produces a hosted run that
skips exactly the jobs that passed locally, and the run's address is recorded. `ci-local status` works on
the desktop after its switch.

## M3. `schiffy91/drive`

Moves:

| From | To |
|---|---|
| `config/pkgs/utils/drive/` sources, `btrc.toml`, `btrc.lock`, `readers.deny`, `status/icons.py`, `testing/` | `src/` |
| `config/pkgs/utils/drive/package.nix` | `nix/package.nix` |
| `options.nix` and the unit-making part of `default.nix` | `nix/module.nix` as `services.drive` |
| `config/pkgs/utils/drive/PLAN.md` | `PLAN.md` |
| `tests/drive/*` except `module`, the T3.1 scenarios included | `tests/` as flake checks |
| `tests/drive/module` | split: the module's own test goes; the adapter's test stays |

- `services.drive` carries every option `settings.apps.utils.drive` has today, plus `user`, plus a
  read-only `statePaths`. Unit names, `/var/lib/drive/<user>`, `/run/drive` and `~/.config/drive` do not
  change, so no machine adopts or downloads anything again.
- Here: the adapter above, the `settings.apps.utils.drive` options unchanged, and the Semu and storage
  modules untouched. The moved nodes leave `tests/graph.json`; `drive-module` stays as the adapter's test
  and `storage` no longer waits on a moved node. The ledger records each move.
- The repository's `ci/local.json` has `lint`, `unit` and `vm`; `vm` needs KVM and is local only.

Verify:

- In the new repository every flake check passes, the mutation scenario included (a removed rule makes
  its scenario fail).
- Here `make lint` and `ci/checks.sh` pass, and the unit and closure checks pass for both hosts.
- On each machine, before the switch: 0 held, 0 waiting, nothing uploading. After it: online, door
  connected, the same folder count, 0 to download, the same library file count as the baseline, and
  `drive held` empty. On the desktop `drive-tray` is active.

## M4. `schiffy91/deckpad`

Moves: `config/pkgs/gaming/deckpad` to `src/` and `nix/`, its logo from `config/pkgs/gaming/artwork/deckpad`
to `assets/`, `vpower-usb-host.patch` with it, and `tests/gaming/deckpad` to `tests/`.

- `services.deckpad` carries `enable`, `role`, `port`, `allow`, `trustedKeys`, `usb`, `bluetooth`,
  `controller` and `user`, with read-only `statePaths`.
- The adapter here keeps what belongs to this configuration: the firewall port
  (`settings.networking.ports.tcp`), the Steam shortcut and its artwork (from the input's `artwork`
  package), and persistence.

Verify: the new repository's unit and server checks pass, and the kernel check too if T4 wired it in. Here
the checks pass and the unit and closure checks pass for both hosts. On both machines
`systemctl is-active deckpad.service` and `deckpad status` read the same after the switch as before it. A
real controller session needs someone at the Deck and goes on the waiting list.

## M5. `schiffy91/proton-sge`

Moves: `config/pkgs/gaming/proton/custom` to the repository's root (`package.nix`, `patches/`, `tools/`,
`dev-shell.nix`, `Makefile`, `README.md`) and `tests/gaming/proton` to `tests/`.

- Two reaches out of the folder are cut. `tools.nix` reads the btrc revision from this repository's
  `flake.lock`; it reads its own. `package.nix` takes Rocksmith's `wineasio` files from
  `../../rocksmith/assets`; it gains an `extraFiles` argument and the Rocksmith module here passes them.
- Outputs: `packages.proton-sge`, `packages.tools`, `devShells.default`.
- The name changes everywhere: the Steam tool is `proton-sge-<version>`, the wrappers are
  `proton-sge-<command>`, and `settings.apps.gaming.steam.protonCustom` becomes `protonSge` in every file
  that uses it (Steam, Battle.net, Assetto Corsa, Rocksmith, the Deck's host file).
- The patch series and the GE version do not change in this milestone.
- The build is local only (`ci/local.json`); hosted CI evaluates the flake and runs the tool tests. The
  desktop publishes the result to the LAN cache before the Deck switches.

Verify: the package builds; the same patch files apply in the same order (the build log's list matches
the baseline's); the tool tests pass. Here the checks pass. After the desktop switch with Steam closed,
Steam's `config.vdf` maps app `0` to the new tool name at priority 75 and no entry names the old tool; the
Battle.net launcher window opens under it and is closed again without starting a game. The Deck switches
by substituting, with nothing compiled.

## M6. WoW addon patches become data

Stays in this repository. `addons.nix` stops choosing patches by addon name. A new option,
`settings.apps.gaming.wow.addonOverrides.<name> = { patches; files; tocAppend; }`, holds what is now three
`optionalString (name == …)` branches, and `addons.nix` applies it to any addon.

Verify: the output of every addon package has the same content hash (`nix hash path`) as before the change;
`ci/checks.sh` passes; `wow-addons-install` on the desktop links the same set of folders as before. The
game is not started.

## M7. Status in the system tray

- In btrc's `Library.Tray`: an entry whose text is a label and the first line a command prints, refreshed
  when the menu opens; and an attention state for the icon, held while a command exits 0.
- Here: `settings.apps.utils.nixosctl.tray.status.<label> = { run; attention ? null; }`, merged by modules
  the way `tray.menu` is. Two providers: one line per `settings.disk.storage` volume (open, locked or
  absent, from `nixosctl storage status`) and one for Deck Pad (from `deckpad status`). Drive keeps its
  own icon.

Verify: a new graphical VM scenario, `nixosctl-tray`, opens the menu, reads the status line, changes the
state behind it and reads the new line, and sees the attention icon come and go. On the desktop the tray
unit restarts cleanly and the menu's layout, read over D-Bus and not by clicking, holds the storage and
Deck Pad lines.

## M8. Immutability resets to an empty root

Why: on FRACTAL-NORTH the snapshot each boot starts from dates from 7 June. Its `/etc/static` points at a
store path that is gone, and it holds old `passwd`, `shadow` and `machine-id` files, debugging leftovers in
`/tmp` and two home folders. Activation overwrites it every boot. An empty root is clean by definition and
is new with every generation.

- A setting, `settings.disk.immutability.base`, with values `clean` (today) and `empty`.
- With `empty`: the boot-time tool creates the next root as a new subvolume instead of a snapshot of
  `CLEAN`, restores persisted paths from the persist root and the live volume only, and does not require a
  `CLEAN` to exist. The installer takes no snapshot. `nixosctl persistence materialize --refresh-clean`
  refuses with a message. `nixosctl diff` compares against a marker recorded when boot finishes.
- The tool's version changes, and the version check in `default.nix` accepts the step from `2026.08.22`.
- The QEMU hosts and the VM twins of both real hosts use `empty`. One node keeps proving `clean`.
- For each real machine, a report of every regular file `CLEAN` holds that is outside the persisted paths,
  saved with the evidence and summarised here. On the desktop that list includes
  `/etc/wpa_supplicant/imperative.conf`, `/etc/fwupd/fwupd.conf` and what is in the June home folders.

The real machines stay on `clean` and their `CLEAN` snapshots are not touched. Moving them is the owner's
decision after reading the reports, and takes effect at a reboot.

Verify, on one commit and by the ledger's current names: every node that covers immutability
(`immutability-hibernate` included, and the assertions fixed in T0.2), the install and reset chains,
`nixosctl-diff`, password change, the `@home` reset and bad-generation scenarios from T3.2, and both
hosts' boot chains. The `persistence-ownership` check passes. The one `clean` node passes too.

## M9. `schiffy91/immutability`

- Moves: `config/system/storage/immutability` (tool and module) and, from `nixosctl`, `lib/btrfs`
  (`DiffScanner`, `SnapshotManager`, `PersistenceManager`, `BtrfsImmutabilityManager`), which become the
  tool's own `diff`, `materialize` and `snapshots` commands. `nixosctl diff` and `nixosctl persistence` run
  them. The import of `nixosctl`'s `NixosLog.btrc` is replaced by the tool's own logging.
- The module's options are its own (`immutability.device`, `.volumes`, `.snapshotsSubvolume`, `.base`,
  `.mode`, `.generations`, `.persist.paths`, `.persist.neededForBoot`). The adapter here fills them from
  `settings.disk.*`, which hosts keep setting.
- Tests in the new repository: the two unit suites, and VM scenarios on a small btrfs-root node for reset,
  files, generations and restore, materialize, orphans and persisted owners, each for both bases. The
  install and host scenarios stay here as tests of the adapter.

Verify: the new repository's checks pass. Here, on one commit, every node named in M8 passes and the
closure check passes for both hosts. The real machines are switched only after that. The boot-time tool
runs at boot, so its first run on real hardware is the owner's next reboot and goes on the waiting list;
the previous generation stays selectable in the boot menu.

# Finish

Measured on this desktop, on the final commits:

- NixOS: the x86_64 required graph under 35 min after a source change and under 3 min with nothing
  changed; `make lint` under 8 min; `make unit` after a one-file edit under 30 s.
- btrc: `make test-fast` under 5 min; `make test` under 30 min.
- `make graph-required` passes in one run on the final commit of this repository.
- The `nightly` and `aarch64` graph sets and btrc's `make test-nightly` each pass once.
- Hosted CI is green on the final commit of every repository, or each red job is shown to fail the same
  way on a commit this plan did not make.
- The ledger accounts for every node and check that was required at T0.
- A final report: a before-and-after timings table, and anything unfinished with the reason.
- The READMEs here and in each new repository say where things live and how a host turns a product on.
  This file is deleted. Memory is updated.

# Waiting for the owner

Filled in as the work runs. Known from the start:

- Running the Gaming Mode sleep-lock nodes on the Deck (T3.3): `make -C tests graph-deck` in `/etc/nixos`
  there. Three are expected to pass. The two real-suspend nodes (`sleep-gamescope-native-suspend-control`,
  `-resume-lock`) fail at game input after wake, a limit of the VM's GPU pass-through recorded in
  `tests/sleep-lock/gamescope-native/README.md`; fixing them needs runs on the Deck, so that is waiting too.
- A schedule for the `nightly` sets, once the CI machine exists.
- mbp21: `git pull` and `nix-update`, if it asks for a password; and the `aarch64` set there, if it needs one.
- A controller session on the Deck after M4.
- The decision to move FRACTAL-NORTH and STEAM-DECK-OLED to the empty base, after the M8 reports.
- The first reboot of each machine after M9.
- Deleting the stale `__pycache__` directories and `tests/nixd` in the checkout at `/etc/nixos` (T4): they
  are not in git, so removing them is the owner's call.

# Not in this plan

- The VM graph runner stays in `nixosctl`.
- WoW's addon engine does not get a repository: after M6 it can, but 30,000 of its lines are captured
  settings that belong to this configuration.
- Sunshine's patches, the Plymouth keyboard, the sleep controller, opportunistic unlock and the secure-boot
  keys stay. They are this configuration's system layer or patches that belong upstream.
- `/etc` as a build product (`system.etc.overlay`). It needs the systemd initrd, and the unlock and
  Plymouth work sit in the initrd.
- Drive on macOS.
