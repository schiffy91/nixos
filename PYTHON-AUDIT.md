The migration replaces repository-owned Python application and test code with
compiled BTRC. Eliminating Python from the entire development/build dependency
closure remains a separate objective: the pinned upstream compiler/LSP and Wine
generators still use it.

The implementation uses `tests/vm/Lab.btrc` and `Guest.btrc` for owned VM/network
lifetimes, bounded serial commands, QMP, boot, initrd and input operations.
`tests/vm/test.nix` evaluates the existing NixOS node modules and compiles the
scenario directly; it does not build or execute the Python test driver.
`Framebuffer.btrc` copies actual EGL/VNC pixels into `Library.Image` observations.
The TLS fixture, affinity snapshot and signal/process probes use typed native
headers with explicit ownership and byte-copy metadata. Test policy stays in
BTRC; generated command fixtures are compiled programs.

The host checks, guest contracts and VM scenarios retain their original negative
cases. CI no longer installs Ruff, Pyright, Pillow, pexpect or vncdotool, and the
Python editor configuration and test-driver stub are removed. The image decoder
uses a small pinned-stdlib compatibility patch for libjpeg-turbo 3.2, shared by
the compiler and language server through
`config/pkgs/dev/btrc/compat.nix`. A second upstream compatibility patch restores
stdlib provenance in the LSP, so typed native byte copies are checked against
the real `Library.Bytes`, including after loading cached declarations.

The inventory below records the **pre-migration audit**, rather than the current
source tree. Verification results are recorded after that historical inventory.

Audited November 8, 2026 against the current working tree, including staged and
untracked files and excluding deleted files, caches, VM state, build output and
vendored dependencies. HEAD was `d3a55aa`; there were substantial existing local
changes. BTRC was inspected at the flake's pinned revision
`f80c15cfb8cb8a0345fe4980a88a0cc87916f1cc`, including its language guide, grammar,
stdlib implementations, native manifests and compiler packaging. This is an
audit and migration design that originally preceded implementation.

**Original inventory.** There were 34 `.py` files under `tests/`, totaling 3,828
physical lines, plus the 17-line `tests/nixos_test.pyi`. There were no
`.py` source files under `config/` or `ci/`. However,
[hdr-screencast-test](config/pkgs/gaming/sunshine/hdr-screencast-test/hdr-screencast-test)
was a 179-line extensionless Python program under `config/`. Its
[package.nix](config/pkgs/gaming/sunshine/hdr-screencast-test/package.nix)
also ran an inline Python program to import its `Probe.build` method.

Of the 34 test files, 19 are standalone host checks/helpers (1,456 lines), four
are programs executed inside guests (561 lines), and 11 are VM-driver scripts
or an inserted script fragment (1,811 lines). Physical line counts are an
inventory, not an effort estimate.

There were **16 Nix files assigning `testScript`**: 14 direct `runNixOSTest`
definitions and two inherited variants. Six contain inline driver code rather
than simply reading a Python file: boot fallback, 1Password, installer secrets,
password files, Sunshine credentials and Sunshine SSH. Removing `.py` files
does not remove those programs or the Python driver. The Secure Boot backup
test also generates a Python tar-writing program inside a guest. Several host
tests generate executable Python command fixtures.

**Migration inventory.** Paths below are relative to `tests/`. Scope describes
the work required to retain existing behavior, including negative checks.

| Files | Lines | BTRC destination and work |
| --- | ---: | --- |
| `checks/chrome_launchers.py` | 114 | CLI contract suite using filesystem fixtures, JSON and independent desktop-entry observations. Preserve translations/actions, unmanaged files, symlinks, ambiguous matches and the no-write-on-repeat timestamp assertion. Small. |
| `ci/publish.py`, `ci/lsp_shutdown.py` | 161 | Separate publishing suite and executable command fixtures; real LSP fixture peer with JSON framing, exit ordering, pipe-hangup observation and child cleanup. Publishing is small; LSP needs process support qualification. |
| `gaming/affinity.py` | 27 | Launcher suite plus an independently executed CPU-affinity/argv/environment probe. Use a typed Linux affinity boundary; retain the exit-37 case. Small after that primitive. |
| `gaming/artwork.py` | 25 | Decode the actual pinned assets through `SystemImageDecoder`; inspect dimensions and alpha extrema in BTRC. Require PNG format, successful full decode and both transparent/opaque pixels. Small; native codec packaging needed. |
| `gaming/proton/tools/test.py` | 95 | Process/filesystem suite plus compiled command fixtures. Retain descriptor inheritance, bounded launch, bad interpreter and development-link restoration checks. Medium. |
| `gaming/runtime/test.py` | 282 | Split credentials, TLS SSH login, virtual display and Slopsmith lifecycle into domain suites. Filesystem cases are straightforward; real TLS server, concurrent writes and signal cleanup are the significant support work. Medium/large. |
| `gaming/wow/test.py`, `gaming/wow/install.py` | 226 | Addon-refresh and installation suites with compiled command fixtures. Use existing file locks and snapshots; preserve lock contention, atomic failure, ownership, inode identity, dangling links and manual replacements. Small/medium. |
| `harness/source-hash/check.py`, `harness/timeout/check.py`, `harness/timeout/blocked.py` | 147 | Exercise the real graph CLI with generated JSON and real child processes. A deliberately stubborn process-tree fixture needs signal control; preserve PID/start-time identity and independent cleanup. Medium. |
| `host-picker/test.py` | 59 | Package the existing `Driver.btrc` with Nix, then drive its actual stdin/stdout. Preserve real registry evaluation, invalid input, EOF and separate Nix diagnostics. Small. |
| `hosts-boot/assert_panel.py`, `assert_responds.py`, `assert_health.py` | 184 | `PanelEvidence` and `BootHealth` helpers. Use existing PPM decoding and image diff; keep the 12-channel-delta/40-changed-pixel contract and all health markers. Small and a good first conversion. |
| `immutability/feature-disable/test.py` | 96 | BTRC scenario over a real disposable Btrfs disk. Keep enabled/disabled/retained profiles, quarantine, fresh re-enable and explicit recovery. Logic is small; needs VM fixture provisioning. |
| `networking/roaming/test.py` | 55 | Two-machine scenario, preserving actual client reachability across NetworkManager profile/subnet changes. Replace Python HTTP services with BTRC HTTP listeners. Requires multi-machine networking. |
| `nixosctl/installer-secret/test.py`, `password-file/test.py` | 250 | Guest BTRC executables using `PTYProcess` and filesystem/process primitives. Keep real PAM/sudo, unsafe-path rejection, concurrent initialization, SIGKILL and reboot cases. Medium; extend PTY control only where needed. |
| `nixosctl/sunshine/test.py`, `sunshine-ssh/test.py` | 311 | Guest service integration suites. Preserve real HTTPS, headers/cookies, CSRF origins, signature/challenge replay, permissions and reboot behavior. HTTP response metadata and test TLS policy need an adequate API. Medium. |
| `opportunistic-unlock/boot/test.py` | 303 | Stateful BTRC scenario with real initrd, LUKS, TPM, hotplug/device identity and bounded worker checks. Preserve bad/expired handoffs, inherited SIGCHLD, device replacement, I/O failure and slow-device isolation. Large fixture/harness work. |
| `plymouth-keyboard/test.py`, `displays.py` | 395 | Typed keyboard/display cases rather than source-text insertion. Keep rotation, scale, layouts, real LUKS prompt, touchscreen mapping, held-key cancellation and DRM unbind/rebind. Needs initrd control plus pixel transform helpers. Large. |
| `recovery/mount.py`, `commands.py` | 98 | Compiled mount-command fixture and BTRC orchestration suite. Keep mount ordering, idempotence, mismatched existing mounts and required/optional devices. Small. Real recovery graph coverage remains necessary. |
| `secure-boot/crypto.py` | 38 | BTRC suite driving OpenSSL as an independent oracle, plus a small inherited-signal fixture launcher. Keep PEM/DER, mismatched/encrypted/corrupt keys, secret-output rejection and actual SIGALRM termination. Small/medium. |
| `secure-boot/backup/test.py` | 115 | Two-machine backup/restore scenario, encrypted storage and real pinned SSH. Replace embedded tarfile code with independent malformed USTAR fixtures or a small fixture encoder. Requires networking and stream-failure preservation. |
| `secure-boot/removal/test.py` | 308 | Firmware/state-machine scenario using actual EFI variables, independently signed recovery payloads, retained generations and repeated boots. Large qualification burden, largely ordinary BTRC logic. |
| `sleep-lock/gamescope-native/control.py`, `gamescope-native-lock/test.py`, `gamescope-native-suspend-control/test.py`, `gaming-plasma/test.py` | 539 | Shared framebuffer/input observations with separate control, lock, suspend and Plasma scenarios. Preserve native GPU/VNC capture, OCR, VT/input ownership, stream blocking, failure lockout and real s2idle/resume. Large; hardware-dependent proof required. |

**The BTRC that should result.** Keep tests named after their domain and their
observable contract. Build executable fixtures once with Nix; configure them
with typed arguments or JSON data. Avoid writing miniature programs into
strings at runtime. A fake `nix` or `curl` is appropriate for command-selection
tests, but actual TLS, mounts, PAM, EFI and input delivery still need their real
integration paths.

Use `string`, f-strings, `Bytes`, collections, structured JSON, exceptions and
ordinary classes in scenario code. Use `ChildProcess.run` for argument-sensitive
execution and `UnixShell` for actual shell expressions. Configuration belongs in
Nix and JSON. Nix should compile and stage programs; the HDR package's protocol
generation and C probe compilation belong in its Nix build, not a BTRC program
that recreates a build system.

Keep expectations and observations immutable at the public API. Construct a
`PanelEvidence`, `BootHealth`, `CommandInvocation` or `FileObservation` once,
then expose queries/getters. Mutable state belongs in explicit owners such as a
VM session, PTY session or temporary fixture directory. The pinned compiler
supports private fields and getter-only properties; a negative compilation
probe confirmed assignment to a getter-only property is rejected. This is not
a claim of deep immutability: returning a mutable `Vector`, `Map`, `Bytes` or
`Image` still exposes mutable state. Return copies, projections or query
methods where isolation matters. There is no need to invent a language-wide
freezing mechanism before beginning this migration.

The storage immutability code is also a useful architectural reference:
[ImmutabilityPlanner](config/system/storage/immutability/src/lib/ImmutabilityPlanner.btrc)
builds domain plans and
[ImmutabilityFs](config/system/storage/immutability/src/lib/ImmutabilityFs.btrc)
owns filesystem effects. Use that separation for expectations and observations.
Its public mutable records are not evidence of enforced immutability. Retain
the boot/initrd ownership of CLEAN/reset recovery; test migration must not
reintroduce live baseline recapture during updates.

For native ownership,
[UserAccount.btrc](config/pkgs/utils/nixosctl/lib/accounts/UserAccount.btrc)
and its [manifest](config/pkgs/utils/nixosctl/btrc.toml) illustrate the useful
shape: an owned native record, declared borrowing/release rules, copied string
results and a small BTRC domain value. The account value's public fields could
become getters if it should be immutable. Its adapter currently performs NSS
buffer management; this does not justify putting scenario decisions into C.

**Existing capabilities and actual gaps.** These findings come from the pinned
stdlib source, not assumed APIs.

| Need | Already available | Remaining work |
| --- | --- | --- |
| Files and fixtures | `TemporaryDirectory`, `DirectoryHandle`, `FileHandle`, `PrivateDirectory`, `AdvisoryFileLock`, `FileSnapshot` | Use the owners and checked close outcomes. Snapshot equality already covers inode/version identity. UID/GID/link-count or exact timestamp observations may need a focused public metadata API; do not duplicate `stat` plumbing across tests. |
| Bounded command execution | `ChildProcess.run`, separate streams, stdin, environment changes, descriptor mappings, timeout and terminating-signal results | Streaming asynchronous child ownership is fragmented between repository `PipeProcess` and `BackgroundProcess`. Consolidate a managed process/pipe API where the actual suites need it. |
| Terminal interaction | `Library.Terminal.PTYProcess`, monotonic deadlines, byte reads, echo checks and owned cleanup | A small exact-prompt session helper can replace most pexpect use. Preserve EOF versus timeout, output bounds and secret-safe diagnostics. Targeted signaling/PID observation needs qualification or API extension for kill tests. |
| Images | `ImageIO` PPM/BMP, `Image.pixel`, `crop`, `diff`, `diffRegion`; `SystemImageDecoder` PNG/JPEG providers | PNG builds need the Linux provider's libpng and libjpeg-turbo dependencies. Add small BTRC rotation/nearest-neighbor sampling/histogram helpers if needed; Pillow is not a reason to implement codecs again. PNG evidence output may need an encoder or an explicit evidence-format change. |
| HTTP | Bounded `HTTPServer`, request parsing/framing; curl-backed `HTTPClient` | The server is plain TCP, not TLS. The client response exposes status/body/error, not response headers, and lacks the TLS controls these tests need. Add a real TLS transport for the fixture server and an HTTP API exposing headers/cookies and explicit test trust policy. |
| Low-level process fixtures | Process status, descriptor mappings, timers and existing native bindings | CPU affinity observation, inherited masks/dispositions, pipe hangup and stubborn descendants need narrow typed POSIX APIs or tiny fixture executables. Avoid arbitrary managed callbacks between fork and exec. |
| GPU console evidence | Existing QMP path, image primitives, external Tesseract | No reusable VNC client was found in the inspected stdlib. Use a typed libvncclient adapter or a narrowly scoped BTRC RFB client. A captured frame must be complete and bounded before publication. |

Prefer direct typed native imports from real headers. Where an API cannot be
expressed safely, use a small adapter with explicit lifetime/error/buffer
contracts. The compiler already supports unique resources, copied results,
borrowed parameters and generated adapters. Its native-interop design still
documents limitations, including unmanaged `void*` handles; qualify a proposed
library binding rather than assuming every C API is automatically safe.

For the TLS fixture, BTRC should own challenge selection, JSON, request history
and assertions; libssl should own TLS contexts/connections and cryptography.
Preserve the test using two certificates with the same public key, and prove
that rejected certificates receive no HTTP request. An HTTP mock or SPKI-only
check would weaken the existing test.

For VNC, native code should only manage the client/event boundary and copy a
completed pixel buffer. Pixel checks, timeouts, OCR invocation and lock-state
assertions stay in BTRC. The existing Gamescope tests record a specific QEMU
`screendump` failure for GPU scanout and then use EGL/VNC readback. Replacing
VNC with ordinary `screendump` would lose the path those tests deliberately
exercise.

Keep the HDR probe's existing `probe.c` as an honest native Wayland/PipeWire
probe during Python removal. Move mode selection, process supervision and
result interpretation into BTRC. Moving all probe logic unchanged into a
`.btrc` file would improve nothing. A later typed Wayland/PipeWire migration can
be justified by reusable APIs and verified callback/resource lifetimes.

**Replacing the VM driver is the substantial project.** The existing
[VmTestRunner](config/pkgs/utils/nixosctl/lib/simulator/VmTestRunner.btrc)
already supplies graph integration, SSH guest commands, serial input, QMP,
reboots, snapshots, copying and state recording. It is a strong base, but its
spec holds one VM and the QEMU builder creates one user-network connection with
SSH forwarding. It does not yet provide the topology and fixture/control
contracts of all the Python tests.

The required additions are:

1. A Nix-built fixture descriptor containing system closures, specialisations,
   disks, firmware, initrd configuration and device topology. Reuse the
   existing Nix guest modules; avoid porting their configuration into BTRC.
   Inspect the pinned NixOS test infrastructure before choosing which VM
   construction pieces to reuse. This audit did not prove a drop-in launch
   adapter for its generated VM scripts.
2. Named VM ownership with isolated shared networking, simultaneous lifetime
   and cleanup. Networking roaming and Secure Boot backup require two actual
   machines; sequential graph dependencies do not replace that topology.
3. A control channel available during initrd and network failure. Several
   current tests execute commands at the real LUKS prompt or alter networking.
   Their control path must not depend solely on guest SSH. Use a bounded serial
   or virtio channel with explicit command/result framing and reconnect rules.
4. Bounded waits for units, files, console evidence, shutdown and a new boot
   identity. Add persistent typed QMP handling where event correlation,
   suspend/wake and device operations demand it. The present QMP helper sends
   JSON through a one-shot `socat` process; it is not a full event session.
5. GPU options, multiple displays, USB/SD/NVMe devices, TPM, framebuffer capture
   and input delivery. Keep raw device configuration in typed fixture data and
   protocol behavior in small BTRC owners.
6. Failure artifacts and teardown equivalent to the current driver: console
   and QEMU logs, screenshots, relevant guest journals, bounded stop/reap and
   removal of fixture-only resources. Preserve graph source hashing and
   parent-state invalidation.

Use JSON for declarative scenario composition. When a test branches on typed
observations or has a substantial state machine, call a compiled BTRC scenario
through the graph rather than extending JSON into a programming language or
embedding a huge shell program. Keep a small shared support layer under tests;
promote genuinely reusable process/image/network primitives to BTRC's stdlib.

**Recommended implementation sequence.**

1. Convert panel/boot-health helpers, Chrome launcher tests, publisher, WoW,
   recovery mount and host-picker. These establish the ordinary BTRC style and
   compiled fixture pattern. Build host-picker's driver in Nix rather than
   compiling it from the test. Move HDR build steps into Nix and its runner into
   BTRC; verify it separately on a real GPU.
2. Qualify the missing process observations and async ownership, then migrate
   affinity, LSP shutdown, Proton launch, graph timeout, crypto signal tests
   and concurrent runtime checks. Preserve independent fixture processes and
   explicit lifecycle observations.
3. Convert the four guest scripts using PTY/process/HTTP helpers. A temporary
   minimal Python driver can still boot and invoke those executables, but label
   that as an intermediate step with Python still required. Add the real TLS
   server fixture and migrate the host Sunshine login tests.
4. Extend the BTRC simulator and first prove a small fixture such as 1Password
   and the real-Btrfs feature-disable scenario. Then migrate two-machine tests,
   boot fallback, Secure Boot and opportunistic unlock. Retain both old and new
   paths until equivalent observations pass; remove each old path afterward.
5. Finish graphics/input/suspend scenarios with image and VNC support. Qualify
   the full existing layout/rotation/scale/display variants and the actual GPU
   path, not just a headless approximation. Remove the final Python driver
   definitions and Python-only test packaging only after these pass.
6. Remove Python lint/editor/test scaffolding: `ci/tools.nix`'s test environment,
   Python stages in `ci/lib/Checks.btrc`, Python inventory logic when no longer
   needed, `ci/pyrightconfig.json`, `tests/nixos_test.pyi`, `.envrc`'s Python
   environment link and workspace Python settings. Update Nix wrappers, graph
   commands, Make targets and documentation. Keep a source inventory check
   that catches extensionless and embedded Python, not only `.py` files.

**Verification must preserve independent evidence.** BTRC tests and BTRC
production code can share infrastructure without sharing the implementation
being checked. Preserve OpenSSL as the key/fingerprint oracle; use independent
tar fixtures for malformed archives; retain an independent keyboard layout
specification. Do not use production encoders/parsers on both sides of every
assertion. A timeout suite exercising `ChildProcess` must have an independent
outer bound and descendant-identity check; its own broken timeout cannot be the
only mechanism preventing a leaked process.

Run migrated host suites through their existing Nix checks or Make entrypoints.
Run VM scenarios through `nixosctl graph tests/graph.json run ...`, preserving
source hashes and parent dependencies. Disk, boot, recovery and immutability
changes require their real VM paths and architecture coverage, including
x86_64/aarch64 where applicable. GPU/OCR/input tests require the actual fixture
environment. Never replace these with source-string checks or successful
compilation alone.

**What would still use Python after the owned-code migration.**

| Dependency | Evidence and consequence |
| --- | --- |
| BTRC reference compiler | `Makefile`, `shell.nix`, `config/lib/btrc.nix` and `ci/tools.nix` select `btrcpy`. Its pinned flake wrapper executes `python3 -m src.compiler.python.main`. |
| BTRC editor tooling | The pinned `btrc-lsp` wrapper executes `python3 -m src.devex.lsp`; formatter/native-plan tools also use Python. Removing Pyright does not remove the BTRC LSP's interpreter. |
| BTRC self-hosted compiler | The pinned flake offers `btrcc`, so switching consumers is plausible. It is a separate qualification of strict imports, native manifests/link plans, stdlib builds, supported targets and generated output. Its current Nix executable is bootstrapped with `btrcpy`, so switching runtime compiler does not erase bootstrap Python from the build dependency graph. |
| Wine/Proton generators | `config/pkgs/gaming/proton/custom/package.nix` and `tools/Wine.btrc` invoke upstream `dlls/winevulkan/make_vulkan` with Python. These are upstream generators, not repository-owned Python implementations. |
| Explicit developer environments | Python appears in FRACTAL-NORTH and mbp21 dev packages, the agent defaults in `config/system/nixos/users/options.nix`, Proton's development shell and Battle.net's test shell with `dbus-python`. Their removal is a separate environment decision. |
| Other dependency closures | Nixpkgs packages and upstream projects may retain transitive Python. A zero-Python system/build closure requires a closure audit; a source audit cannot establish it. |

**Validation performed for this audit.** A standalone scratch program compiled
with the pinned `btrcpy`, linked as C11 and ran successfully. Its BTRC source
contained no C includes, raw pointers, allocation or POSIX calls. It exercised
PPM encode/decode, image crop/diff with the existing 12/40 threshold,
getter-only evidence, argv containing spaces and literal `$(...)`, separate
stdout/stderr, a bounded child-process call and checked temporary-directory
cleanup. A second compilation intentionally assigned to the getter and failed
with `Property 'changedPixels' has no setter`.

Probe source and artifacts were recorded in `/tmp/nixos-python-audit.kOOPtp/`.
That initial probe established the building blocks; the implementation and
integration runs below supersede its limited validation scope. Switching to the
self-hosted compiler remains separate work.

**Implementation verification (November 8, 2026).** The owned-source inventory
now has zero `.py` and `.pyi` files, Python shebangs or embedded Python test
programs. The sole `testScript` assignment is the empty string required to
evaluate NixOS's VM modules; the compiled BTRC runner owns execution.

The full build/unit portion of `make lint` passed: management programs,
immutable planning, sleep controller/helpers, unlock memory/header contracts,
recovery installation, networking policy, gaming, native headers, publisher and
LSP protocol checks. Initial source diagnostics exposed the codec packaging and
LSP provenance issues described above; both now have focused passing checks.
The final source-only lint passed all 229 Nix and 237 BTRC files through their
parsers/compiler and language servers, with zero error diagnostics. Shell,
JavaScript, Lua, workflow, JSON, TOML and YAML checks also passed.
`nix-build tests/ci ci/native.nix` passed after adding the positive native-copy
and forged-`Bytes` editor protocol tests. `nix-build tests/gaming` passed,
including artwork decoding, capture evidence boundaries, real TLS pinning,
affinity, WoW, Proton and signal/lifecycle cases.

`nix flake check --no-build --all-systems` passed against a temporary snapshot
of all existing tracked and untracked source files. The ordinary Git-flake
invocation cannot see pre-existing untracked manifests in this working tree;
the snapshot avoids staging or modifying those changes. This is configuration
evaluation for all systems, not an aarch64 VM execution claim.

VM tests ran through the graph, using real NixOS guests and source-keyed states:

| Scenario | Passing run |
| --- | --- |
| Host picker; source hashing; process timeout/cleanup | `7ff7a7edea27`; `4f341df906a1`; `f968a8c0666e` |
| Network roaming across two guests | `ac6a84bf279d` |
| Encrypted Secure Boot backup and hostile archives | `cb96f656e41e` |
| Installer secret/PAM and reboot | `34b1f030f28e` |
| Password-file concurrency and filesystem attacks | `24924baa11ce` |
| Sunshine credential provisioning; SSH login/replay | `4261665f7324`; `3716b2e554da` |
| Real Btrfs immutability disable/re-enable/recovery | `8a92f7dcfea8` |
| LUKS/TPM opportunistic unlock and device failures | `1c91492fc322` |
| Secure Boot key removal and recovery boots | `89fcb3cbd378` |
| Boot-generation fallback; 1Password integration | `2208d45bc9c1`; `1cd114484bbc` |
| Gaming runtime contracts; stock Plasma lock | `88b60db335cd`; `57622460a51f` |
| Native Gamescope input/rendering control | `0b1f9e7c21d2` |
| Native Gaming lock; lock without Sunshine | `816b14e1f7eb`; `e7ce3f338a9f` |
| Plymouth touch unlock; rotated output | `5d32846b1d0a`; `5e08627d9c92` |
| Plymouth German; French layouts | `314d29aecda0`; `11a6dcc08868` |
| Plymouth multiple displays and fully headless interval | `15fad21c687e` |

Network roaming, backup and installer-secret were rerun after the final serial
framing change. Plymouth transfers its complete multi-megabyte debug trace;
wrapped base64 and a `Bytes` accumulator avoid quadratic string growth without
discarding the log or weakening the secret-leak checks. VNC captures drain both
socket data and libvncclient's buffered updates before requesting current pixels.
The full Plymouth matrix includes light themes, rotation, German Unicode input,
French Compose input and scale 2. Its multi-display scenario passes real DRM
unbind/rebind, independent touch-device ownership, removal while holding a key,
and preservation of partial input through a completely headless interval. The
driver preserves the original NixOS fixture's `-nographic` launch mode, with QMP
as its sole monitor.
Reboot requests deliberately do not wait for a shell reply: shutdown can stop
the backdoor before it acknowledges a successful request. The following
bounded VM-exit and boot checks establish completion.

Two hardware-path limitations remain explicit failures:

- Native-context real s2idle resumes but loses game input. Both the unlocked
  control (`7a5b72f805f8`) and locked resume (`9816c549e34e`) reproduce the
  previously documented virtio-GPU resource errors. The locked test passes
  password rejection and restores the original VT/PID before the input-marker
  assertion fails. No GPU workaround or weakened assertion was introduced.
- Real headless HDR capture passed `shm-sdr` and `shm-pq`. The `hdr10` path
  failed with no DMA-BUF frames and KWin's
  `GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT` error. The BTRC runner reports that
  failure; it does not count a negotiated format without frames as success.
  Capture logs were recorded under `/tmp/kwin-hdr-logs.Id4AID` (passing modes)
  and `/tmp/kwin-hdr-logs.2rcGqm` (HDR10 failure).

These runs do not establish physical Steam Deck suspend/touch behavior, and
the full aarch64 VM suite and complete host-boot matrix were not rerun.

The subsequent Linux upgrade refresh advanced Nixpkgs to
`f4a03c008967841f2a0b3cb7c436fc6fc31671f6` and Jovian to
`34aa19053d730b023e35cf852976aef86fdce219`; the BTRC pin is unchanged.
With those inputs, `make -j4 build unit`, the Nix-packaged CI, Secure Boot,
recovery, sleep, opportunistic-unlock, networking and gaming suites, and
`ci/native.nix` all passed. The graph's `quick` and `stateful-host` nodes passed
again (`stateful-host@cce85d275c7d`). Source checks passed for 229 Nix and
237 BTRC files, with zero compiler/workspace errors and no BTRC diagnostics.
An all-source snapshot also passed `nix flake check --no-build --all-systems`.
The native x86_64 flake check derivations were then built and passed, including
Chrome launcher behavior and the host, installer, disk and persistence checks.
The installer-secret VM also passed on the refreshed inputs
(`installer-secret@e3f623e2ece1`), exercising real PAM and reboot behavior.
These checks validate the refreshed inputs; they do not establish deployment
or physical-host health.

The full host build exposed a Nixpkgs Cargo-hook regression in Codex 0.167.0:
the hook forces system native libraries that its package does not declare.
`config/pkgs/dev/codex/package.nix` preserves Codex's declared bundled-library
build mode, including its SQLite and prefixed jemalloc. The corrected package
built successfully and passed both upstream daemon-lifecycle and voice-runtime
checks. The package-selection and application-hierarchy checks passed again.
Both physical hosts' Secure Boot system closures then built successfully as
`26.11.20261107.f4a03c0`. Building these closures does not activate either host.
