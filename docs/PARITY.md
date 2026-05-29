# Python to BTRC Parity

This is the migration ledger for the old Python/Rust management layer. VFIO is
intentionally removed from the BTRC rewrite because that path is currently
broken and out of scope.

| Original file | Responsibility | BTRC replacement | Status |
|---|---|---|---|
| `scripts/bin/eval.py` | Evaluate a NixOS config attribute | `bin/nixosctl.btrc` and `src/nix/config.btrc` | Ported |
| `scripts/bin/install.py` | Disko mount/format, install NixOS, secure permissions, create clean snapshots | `src/install/installer.btrc`, `src/nix/permissions.btrc`, `src/btrfs/snapshot.btrc` | Ported |
| `scripts/bin/diff.py` | BTRFS changed-file scan with persist/ignore filtering | `src/btrfs/diff.btrc` | Ported |
| `scripts/bin/nixos/cli.py` | Command dispatch and root escalation | `bin/nixosctl.btrc`, `Platform.isRoot`, `sudoSelf` | Ported |
| `scripts/bin/nixos/update.py` | Update, upgrade, clean, rebuild, post-update immutability convergence | `src/nix/rebuild.btrc` | Ported |
| `scripts/bin/nixos/change_password.py` | LUKS key rotation, PAM account password, hash sync, TPM2 optional refresh | `src/nix/password.btrc`, `UnixPamPassword`, `src/hardware/tpm2.btrc` | Ported |
| `scripts/bin/nixos/secure_boot.py` | sbctl key lifecycle and target switching | `src/hardware/secure_boot.btrc` | Ported |
| `scripts/bin/nixos/tpm2.py` | TPM2 presence, LUKS checks, enroll, wipe, status | `src/hardware/tpm2.btrc` | Ported |
| `scripts/bin/nixos/audio.py` | pactl sink listing and preset application | `src/desktop/audio.btrc`, `src/desktop/labels.btrc` | Ported |
| `scripts/bin/nixos/caffeine.py` | systemd-inhibit sleep toggle and pid file | `src/desktop/caffeine.btrc` | Ported |
| `scripts/bin/nixos/displays.py` | kscreen output parsing, layout, enable/disable/primary/dpms | `src/desktop/displays.btrc`, `src/desktop/labels.btrc` | Ported |
| `scripts/bin/nixos/system.py` | Launch update/upgrade in a terminal | `src/desktop/system_ui.btrc` | Ported |
| `scripts/bin/nixos/daemon.py` | PyQt tray/window frontend for desktop helpers | CLI backends in `src/desktop/*.btrc`, stdlib `Ui*`, `NativeUiBackend`, and `Daemon*` primitives | Backend ported; persistent tray/window loop still needs native/WebView binding |
| `scripts/bin/nixos/labels.py` | Read display/audio labels from TOML | `src/desktop/labels.btrc`, stdlib `Toml` | Ported with TOML parsing moved into the BTRC stdlib |
| `scripts/lib/shell.py` | Shell execution, chroot, filesystem, JSON, git safe-directory helpers | BTRC stdlib `Command`, `UnixShell`, `FileSystem`, `PathTools`, plus local managers | Ported |
| `scripts/lib/config.py` | Local config, secrets, Nix eval, rebuild orchestration | `src/nix/config.btrc`, `src/nix/secrets.btrc`, `src/nix/rebuild.btrc` | Ported |
| `scripts/lib/interactive.py` | Confirm prompts, host selection, password prompts, reboot prompt | `src/core/interactive.btrc` | Ported |
| `scripts/lib/snapshot.py` | CLEAN snapshot creation and subvolume deletion | `src/btrfs/snapshot.btrc` | Ported |
| `scripts/lib/utils.py` | Logging, argument parsing, root checks, reboot | `src/core/log.btrc`, stdlib `CliArgs`, `Platform`, local command code | Ported |
| `scripts/lib/immutability.rs` | Original v1 initrd reset implementation | Live Nix module still references Rust when implementation is `v1` | Kept only for original implementation |
| `scripts/lib/semipermeable_membrane.rs` | Original Rust v2 prototype | `src/immutability/semipermeable_membrane.btrc` and `generated/semipermeable_membrane.c` | Replaced for live `semipermeable_membrane` implementation |

## Test Coverage

| Target | Coverage |
|---|---|
| `make -C btrc build generated` | Transpiles and builds `nixosctl` plus the generated BTRC membrane |
| `make -C btrc graph-early` | Runs source-aware cheap graph defaults, including CLI parity plus TOML/YAML spec parsing |
| `make -C btrc graph-installer-ssh` | Boots the NixOS minimal ISO under QEMU, injects SSH over serial, snapshots the live installer |
| `make -C btrc tpm2-probe` | Boots the NixOS minimal ISO under QEMU with `swtpm`, verifies TPM2 in the guest, and records a graph state |
| `make -C btrc secure-boot-capabilities` | Reports x86_64 QEMU Secure Boot firmware, vars, TPM device, and `swtpm` support |
| `make -C btrc secure-boot-lanzaboote` | Optional full x86_64 Lanzaboote auto-provisioning graph: install Secure-Boot target, first boot generates/enrolls keys, reboot verifies Secure Boot |
| `make -C btrc graph-full` | Reuses the installer state, runs QMP and TPM2 probing, installs reset and disabled permutations, boots installed reset mode, and verifies persistent-vs-ephemeral reset behavior |
| `make -C ../btrc test-unit && make -C ../btrc test-btrc` | BTRC compiler/stdlib tests, including imports, UI/daemon primitives, and `Vector<T>` class-object ownership |

## Remaining Intentional Gaps

| Gap | Reason |
|---|---|
| GUI tray/window daemon | Initial BTRC `DaemonApp`/`Window`/`Tray`/`HtmlView`/`NativeUiBackend` stdlib layer exists. The remaining gap is a persistent tray/window event loop and real WebView/native widget backend. |
| VFIO | Removed from the BTRC rewrite because the original path is broken. |
| Secure Boot / TPM2 hardware validation | TPM2 and x86_64 Secure Boot QEMU coverage exists; real hardware enrollment still needs final firmware validation outside this VM harness. |
