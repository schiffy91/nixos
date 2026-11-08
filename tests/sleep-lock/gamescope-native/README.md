# Direct DRM Gaming Mode research fixtures

These opt-in graph nodes use pinned Jovian SDDM, PAM, logind, and Gamescope on a
virtual DRM console. A terminal replaces only the Steam account application.
The terminal sets `STEAM_GAME=769` and `GAMESCOPECTRL_BASELAYER_APPID=769`, as
required by Gamescope's Steam-controlled focus protocol. It is not a nested
desktop compositor test.

Run from the repository as an ordinary user:

```sh
make build
env -u SSH_AUTH_SOCK nix-shell tests/shell.nix --run \
  'NIXOS_CONFIG_ROOT=$PWD ./build/nixosctl graph tests/graph.json run sleep-gamescope-native-control'
```

Choose the explicit graph node for the property being tested:

| Node | Contract |
| --- | --- |
| `sleep-gamescope-native-control` | Direct scanout, selected Steam window, actual pixels, and physical-console keyboard input |
| `sleep-gamescope-native-lock` | Gaming-only root lock, complex touch/keyboard password, cancellation/VT escape denial, failed sleep, controller recovery, overlapping requests, Sunshine service stop/restore, and same-process game rendering/input restoration |
| `sleep-gamescope-native-lock-without-sunshine` | Same Gaming lock with Sunshine absent; missing units do not prevent locking or unlock |
| `sleep-gamescope-native-suspend-control` | Unlocked real kernel s2idle and original game rendering/input after ACPI wake |
| `sleep-gamescope-native-resume-lock` | All lock checks plus real s2idle, password rejection after resume, and authenticated original game rendering/input restoration |

The real-suspend gates are separate from the backend-injection gate. Failure
of a real-suspend gate must remain visible; a passing injected backend does
not prove GPU power-management recovery. These tests do not establish Steam
Deck hardware behavior or concealment of Gamescope's PipeWire capture stream.
The Gaming policy stops the configured Sunshine services; arbitrary capture
clients are outside that service boundary.

The runner requires exactly one accessible AMD `amdgpu` render node and KVM.
It refuses ambiguous AMD selection and does not change host permissions or
open host DRM primary nodes. QEMU uses the experimental AMD native context
renderer: a driver hang/reset can affect other clients on that same host GPU.
The driver is built in Nix's normal sandbox and then executed unprivileged
outside it. No authentication agent is used. Each run uses two virtual CPUs,
4 GiB RAM, a 600-second process timeout, and a 1,800-second graph timeout.
Its private scratch and VM disks are removed on exit; graph logs and images
remain under `.vm/e2e/`.

QEMU's EGL headless console is observed using its own VNC listener on a Unix
socket inside private scratch, with `vncdotool`; there is no TCP listener or
guest VNC server. QMP injects keyboard events. Guest files and OCR of actual
console pixels jointly prove input and rendering. The HMP `screendump`
response is recorded because QEMU 11.1.2 rejects GL scanouts with `no surface`.

Relevant pinned sources:

- [QEMU native-context GPU requirements](https://github.com/qemu/qemu/blob/v11.1.2/docs/system/devices/virtio/virtio-gpu.rst)
- [QEMU EGL headless readback](https://github.com/qemu/qemu/blob/v11.1.2/ui/egl-headless.c)
- [Gamescope Steam focus selection and X11 properties](https://github.com/ValveSoftware/gamescope/blob/3.17.0/src/steamcompmgr.cpp)
- [Linux virtio GPU freeze/restore](https://github.com/torvalds/linux/blob/v7.3/drivers/gpu/drm/virtio/virtgpu_drv.c)

## Gaming-only policy results (2026-11-07)

The production Gaming-only controller passed `sleep-gamescope-native-lock`
run `872d6db09483` (93.49 seconds) and the absent-Sunshine variant
`da7b79e1a6ab` (93.73 seconds). Both drivers completed cleanup. These runs used
Git tree `bcaf0b3f7bf8b809c510a4a72469f1d9b6681b9a`; subsequent edits adjusted
only the separate stock-Plasma regression fixture, documentation, graph gate,
and whitespace/comments in the Gamescope fixture.

The public fixture password is `aB3! z`. Touch input exercises Shift, digits,
punctuation, space, and backspace; wrong passwords and cancellation leave the
console locked. A failed backend and controller restart require fresh password
input before restoring the same compositor and interactive client. Overlapping
requests invalidate an earlier authentication attempt. A new logind Plasma
session appearing during the lock does not depend on custom KDE methods.

Real systemd user units run harmless fixture processes for the two configured
Sunshine names. Both are stopped before readiness, ordinary starts are skipped
while locked, and only previously active units are restored after authentication.
The second variant has no Sunshine units at all. No paired-stream behavior is
claimed by these service tests.

The portable `sleep-gaming-plasma` regression also passed (`fdd8ee85937f`):
using tree `7e7727456628a6c1f1d3dc2cc94d153444c16318`. Ordinary Plasma
sleep stays on its original console, the stock KDE greeter
rejects a wrong password and accepts the complex account password, and no root
Gaming lock or Sunshine marker is created. It is part of the required graph;
the AMD-native Gamescope nodes remain explicit opt-in tests.

The existing full-policy `sleep-lock-authentication` regression also passed
(`e189a7e14188`, 358.86 seconds; cleanup at 359.35 seconds) using that same tree,
including its existing s2idle, interrupted request, and expired-account checks.
Its unlocked sleep-control dependency passed as `1e3d9d2e0379`.

No new Gamescope real-suspend test passed. The earlier GPU power-management limitation
below remains. Physical OLED touch rotation, Steam controls, suspend/resume,
and game preservation need acceptance on the actual Deck.

## Earlier full-policy results (2026-11-07)

| Run | Result and evidence |
| --- | --- |
| Native control `8824ae4f807b` | PASS, 26.98 seconds. Actual 1280×800 scanout, selected app 769, visible terminal markers and QMP keyboard input. |
| Native lock `202882716a41` | PASS, 49.22 seconds. Wrong password and VT escapes denied; touchscreen password accepted; failed suspend still locked; keyboard password accepted. Both releases restored the same Gamescope PID and fresh OCR/input markers. |
| Initial combined lock/resume `b79271e1bd00` | FAIL, 74.71 seconds. Non-PM checks passed; real s2idle resumed to the password keyboard and rejected a wrong password. The correct password restored the original VT/PID, but the game stopped accepting input. |
| Unlocked real-suspend control `43491271eb68` | FAIL, 43.89 seconds. Without the lock enabled or called, s2idle/wake lost game input and QEMU's actual captured frame read “Display output is not active.” |

The two real-suspend failures contain virtio GPU `SET_SCANOUT`/`RESOURCE_UNREF`
and blob-resource failures with `ERR_INVALID_RESOURCE_ID`. Linux's virtio GPU
freeze path resets the device; QEMU's virgl reset discards renderer resources.
The unlocked reproduction identifies a native-context VM power-management
failure independent of the root lock. It does not establish real Steam Deck
suspend behavior. No kernel, QEMU, Mesa, or Gamescope patch was added to hide it.
The separate `sleep-gamescope-native-resume-lock` gate retains the complete
real-suspend assertion and was not rerun after splitting the original test.

These runs used the repository's `f2798b0` base plus this fixture. The non-PM
pass and unlocked suspend failure used frozen fixture diff SHA-256
`fd9d2888515ef9d1e69d3edc3cbe9c7c32d7c9052244c72b513f59c0c9215089`;
this results section was updated afterward. Logs and PNGs remain in each
named `.vm/e2e/<node>-<id>/` directory of the test checkout.

Every run's QEMU, driver, timeout, and virtual network child exited, and its
private scratch was removed. The passing control and non-PM lock completed
cleanup at 35.08 and 57.25 seconds respectively. No test or build is left
running by this fixture work.
