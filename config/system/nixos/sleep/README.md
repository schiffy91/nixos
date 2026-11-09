# Sleep password authentication

## Gaming Mode sleep password

This module supplies account-password locking for an already-running Gamescope
session. Systemd/logind still perform suspend and hibernate; Plasma keeps its
normal screen locker. The pinned Gamescope lacks the session-lock protocol used
by ordinary Wayland lockers. A login greeter alone does not hide or lock the
running game's console.

`settings.sleep.lockGamingSession` uses the existing Plymouth touchscreen keyboard
and Linux-account PAM authentication for sleep requested while the configured
user has a Gamescope session. The Deck enables this policy; Fractal does not.

The game keeps running in its existing session. Sleep preparation switches to a
locked console and hides the game's scanout before allowing the sleep backend.
After wake, enter the Linux account password with touch or a physical keyboard.
The keyboard supports letters, Shift/Caps, digits, punctuation, spaces and
backspace. Wrong input, Escape, a cancelled attempt, an interrupted controller,
and a failed sleep backend do not release the console.

Desktop Mode keeps native Plasma idle and resume locking. The Gaming Mode policy
neither patches KDE nor routes ordinary Plasma sleep through the console UI.
If a desktop session appears while a Gaming lock is already held, fresh account
password authentication still gates return to the physical console.

Configured Sunshine services are stopped before sleep is admitted. Their start
condition prevents normal restarts while locked. After password authentication,
only services that were previously running are restarted. Consequently an active
stream disconnects on sleep; reconnect after local unlock. This boundary covers
the configured `sunshine.service` and desktop-app service, not arbitrary same-user
capture programs or manually launched binaries.

These hooks cover suspend, hibernate, hybrid sleep and suspend-then-hibernate.
They do not add a Steam menu/manual or Gaming Mode idle-lock action. Steam's own
PIN lock is separate. The Deck selects ordinary deep suspend rather than
redirecting suspend to hibernation. Its 2026-11-08 display-readiness failure was
traced to AMD plane degamma conflicting with Plymouth's legacy CRTC gamma.
The display-state reset below fixes that handoff; physical sleep/wake acceptance
still requires deployment and a real suspend test.

Direct Gamescope VM tests use the real compositor, DRM, Xwayland and kernel input,
with an interactive terminal standing in for the signed-in Steam application.
They verify `aB3! z` entered through touchscreen Shift, number/symbol keys, space,
and backspace, plus physical-keyboard authentication and return to the same
running compositor/client. Service tests exercise actual systemd user-unit
stop/start conditions with harmless fixture processes, not a paired stream.
Real Steam controller/UI behavior, OLED touch rotation and real hardware suspend
still need physical validation. The existing native-context VM GPU loses state
on real suspend even in its unlocked control; no new passing suspend claim is made.

The Gaming policy is deployed on the physical Deck. The owner verified touch
password authentication and return to Gaming Mode after the conflicting AMD
color settings were manually reset. The automatic reset has passed the native
Gamescope VM regressions; its physical deployment and real suspend test remain
pending authentication. Readiness failures are logged through syslog without
tracing password input. Native Plasma automatic locking is also enabled.

## Controller boundary

The root controller holds VT63 while Plymouth provides the password prompt.
It disables keyboard SysRq temporarily, checks the kernel console-switch lock,
and waits for the DRM scanout readiness check before admitting sleep. A separate BTRC
PAM process receives the password through a private pipe; the BTRC controller
receives only authentication success or failure. Root-only state in `/run`
contains the account UID, original VT/SysRq setting, pending sleep requests,
authentication generation and previously active Sunshine units. It contains no
password or hash.

`SleepState` owns typed phases, pending requests and authentication attempts;
JSON is only its persistence format. The controller uses standard-library
filesystem leases, process execution and timers. `LockedConsole` and
`DisplayCard` own the native resources; closing either never authorizes unlock.
The remaining native boundary is VT ioctls, DRM blank/readback and PAM.

After every CRTC and plane is confirmed blank, `DisplayCard` resets supported
color/HDR and rotation properties in a single atomic commit and verifies their
values. Disabling scanout alone leaves these properties behind: on the Deck,
Gamescope's AMD plane degamma conflicts with Plymouth's legacy gamma setup and
causes the driver to reject its modeset. Unknown and immutable properties are
left alone. This reset never restores the game's framebuffer or authorizes an
unlock; Gamescope restores its own output after authenticated VT release.

`PasswordResponse` owns a fixed standard-library buffer that is wiped on close.
`PamResponses` owns native responses until PAM accepts them. The password is
never converted to a managed string; ordinary metadata uses BTRC classes and
strings. The dedicated PAM process locks memory, disables core dumps and ties
its Plymouth child to its lifetime. It links PAM without DRM; the separate
scanout helper clears CRTCs and planes and verifies the result. The three native
headers contain declarations only.

Each new request or controller recovery invalidates earlier authentication.
A correct password releases the console only after every pending sleep operation
has finished. Failed preparation, cancelled sleep and controller failure retain
the console lock. An interrupted release requires fresh authentication.

KDE uses its standard locking behavior. This module does not patch KWin or
KScreenLocker, add a root desktop-bus interface, or drive a second Plasma lock.
The logind query only identifies whether the configured user has a Gamescope
session before starting a Gaming lock.

## Regression tests

Build the CLI with `make build`, then run the graph nodes:

```sh
nix-shell tests/shell.nix --run 'NIXOS_CONFIG_ROOT=$PWD ./build/nixosctl graph tests/graph.json run sleep-gamescope-native-lock sleep-gamescope-native-lock-without-sunshine sleep-gaming-plasma'
```

The [direct Gamescope fixtures](../../../../tests/sleep-lock/gamescope-native/README.md)
retain separate baseline and real-suspend controls. The stock Plasma regression
checks a wrong and correct mixed-character password and confirms that desktop
sleep creates neither Gaming lock state nor its service-start blocking marker.
