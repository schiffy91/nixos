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

The five nodes are the graph set `deck`: `make -C tests graph-deck` runs them all on a host with one AMD GPU,
which among this configuration's machines is the Steam Deck. They are not in the required set, because the
desktop that runs it has no such GPU. The two real-suspend nodes still fail at game input after wake, as
recorded below; that can only be worked on where they run.

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

The public fixture password is `aB3! z`. Touch input exercises Shift, digits,
punctuation, space, and backspace; wrong passwords and cancellation leave the
console locked. A failed backend and controller restart require fresh password
input before restoring the same compositor and interactive client. Overlapping
requests invalidate an earlier authentication attempt. A new logind Plasma
session appearing during the lock does not depend on custom KDE methods.

Real systemd user units run harmless fixture processes for the two configured
Sunshine names. Both are stopped before readiness, ordinary starts are skipped
while locked, and only previously active units are restored after authentication.
The variant without Sunshine has no such units at all. No paired-stream behavior
is claimed by these service tests.

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
socket inside private scratch, through BTRC's typed libvncclient bridge; there is no TCP listener or
guest VNC server. QMP injects keyboard events. Guest files and OCR of actual
console pixels jointly prove input and rendering. The HMP `screendump`
response is recorded because QEMU 11.1.2 rejects GL scanouts with `no surface`.

Relevant pinned sources:

- [QEMU native-context GPU requirements](https://github.com/qemu/qemu/blob/v11.1.2/docs/system/devices/virtio/virtio-gpu.rst)
- [QEMU EGL headless readback](https://github.com/qemu/qemu/blob/v11.1.2/ui/egl-headless.c)
- [Gamescope Steam focus selection and X11 properties](https://github.com/ValveSoftware/gamescope/blob/3.17.0/src/steamcompmgr.cpp)
- [Linux virtio GPU freeze/restore](https://github.com/torvalds/linux/blob/v7.3/drivers/gpu/drm/virtio/virtgpu_drv.c)

## Mesa 26.3.0 in the guest

With Mesa 26.3.0 in the guest, all five nodes failed on the Deck at the first
frame check, `Missing framebuffer text GAMESCOPE_CONTROL_READY`. The captured
frame was the same in every run: the terminal's white background covered with
small black squares and no text. Gamescope's own screenshot inside the guest
had that content too, so the guest drew it; QEMU's readback was correct.

The guest's Mesa had moved from 26.2.6 to 26.3.0 with the nixpkgs pin since the
last pass. QEMU, virglrenderer and Gamescope had not changed. With the guest's
Mesa set back to 26.2.6 and nothing else changed, the control node passed on
the Deck. On 26.3.0, `RADV_DEBUG=nodcc` with `AMD_DEBUG=nodcc` did not help.
With `XWAYLAND_NO_GLAMOR=1` the first frame was readable and a later one was
not, which places the fault in the buffers the 26.3.0 OpenGL driver hands the
compositor through AMD native context.

The fixture therefore pins the guest's `hardware.graphics.package` to Mesa
26.2.6 from the nixpkgs revision that last passed. Remove the pin when a later
Mesa passes the control node without it. No upstream report has been filed:
[issue 2](https://github.com/schiffy91/nixos/issues/2).

## Real suspend in the guest

`sleep-gamescope-native-suspend-control` and
`sleep-gamescope-native-resume-lock` enter and leave kernel s2idle and then
fail waiting for the game input marker. Before it fails, the locked scenario
displays the password keyboard, rejects a wrong password and restores the
original VT and Gamescope PID after correct authentication. The unlocked
control reproduces the same missing input marker with the Gaming lock
disabled, and QEMU's captured frame has read "Display output is not active."

Both log virtio GPU `SET_SCANOUT`/`RESOURCE_UNREF` and blob-resource failures
with `ERR_INVALID_RESOURCE_ID` after wake: commands `0x10c` and `0x208` fail
with responses `0x1200` and `0x1203`. Linux's virtio GPU freeze path resets
the device; QEMU's virgl reset discards renderer resources. The unlocked
reproduction identifies a native-context VM power-management failure
independent of the root lock. It does not establish real Steam Deck suspend
behavior. No kernel, QEMU, Mesa, or Gamescope patch was added to hide it, and
neither assertions nor production GPU handling were changed to conceal the
failures. This is [issue 1](https://github.com/schiffy91/nixos/issues/1).

virtio does not reproduce the Deck's AMD-specific color properties either, so
these runs validate the shared lock lifecycle and not the display-state reset
the physical Deck needs before Plymouth can light the panel. The
[sleep module's README](../../../config/system/nixos/sleep/README.md)
describes that reset.
