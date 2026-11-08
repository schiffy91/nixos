# Sleep password authentication

## Gaming Mode sleep password

`settings.sleep.lockGamingSession` uses the existing Plymouth touchscreen keyboard
and Linux-account PAM authentication for sleep requested while the configured
user has a Gamescope session. The Deck enables this policy; Fractal does not.
`settings.sleep.lockSession` remains disabled on both hosts. The two policies are
mutually exclusive.

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
PIN lock is separate. The Deck's existing `hibernateOnSuspend` setting is retained;
hibernation can therefore require boot LUKS unlock followed by this account prompt.

Direct Gamescope VM tests use the real compositor, DRM, Xwayland and kernel input,
with an interactive terminal standing in for the signed-in Steam application.
They verify `aB3! z` entered through touchscreen Shift, number/symbol keys, space,
and backspace, plus physical-keyboard authentication and return to the same
running compositor/client. Service tests exercise actual systemd user-unit
stop/start conditions with harmless fixture processes, not a paired stream.
Real Steam controller/UI behavior, OLED touch rotation and real hardware suspend
still need physical validation. The existing native-context VM GPU loses state
on real suspend even in its unlocked control; no new passing suspend claim is made.

The host configuration enables the Gaming policy on the next deployment. It has
not been activated or accepted on the physical Deck by this change. Native
Plasma automatic locking is also enabled in the Deck's account configuration.

## Existing full-session policy

`settings.sleep.lockSession` remains **disabled by default** and is disabled on
both physical hosts. The implementation below includes native Plasma integration
and its earlier VM evidence. Its KDE patches and root user-bus policy are not
installed by the Gaming-only option.

The root controller reserves VT63 before any of the four systemd sleep units
can run their backend. It disables keyboard SysRq temporarily, verifies the
kernel console-switch lock, and waits for the Plymouth display to acknowledge
readiness. The same touchscreen/XKB package used during boot provides the UI.
A dedicated PAM stack checks the configured administrator's Linux password and
account status; SSH-agent authentication cannot satisfy this check.

After acquiring VT63, a separate native DRM helper disables every CRTC and
clears all universal-plane framebuffer/CRTC bindings, checking kernel readback
before Plymouth can re-enable a display. It never saves or restores a private
framebuffer. Plymouth's optional `--has-safe-displays` query independently
enumerates the DRM cards and requires each connected output's current CRTC and
enabled planes to use a mapped Plymouth framebuffer, with DRM master ownership.
Missing renderers and failed mappings, ownership or modesets cannot count as
readiness. The controller repeats this check before sleep acknowledgement and
while supervising the locked session. Ordinary boot clients keep their existing
behavior; `--has-active-vt` alone is insufficient for locking.

If display preparation fails, the sleep backend is denied and VT63 stays locked
while the controller retries. Clearing a renderer fault can restore the password
prompt without first exposing the desktop. A driver that refuses checked
pre-blanking remains an explicit failure: denying sleep cannot prove that
uncooperative hardware has hidden its old pixels. Physical display hotplug and
resume races still require hardware validation.

Passwords travel from Plymouth to a short-lived native PAM process over a
private pipe. That process locks its memory and disables core dumps. Plymouth
also holds transient copies in its client, daemon and password cache; its IPC rejects
non-root clients, and its daemon exits after unlock. The service disables core
dumps for these descendants too. The Python controller receives only an exit
status. Root-only state in `/run` stores the authentication account's UID,
original VT/SysRq setting, pending sleep units, a random request generation and
the native compositor's single-use lock token;
it contains no password or hash. Every sleep request invalidates an outstanding
authentication attempt. A successful password may release the console only if
its generation is still current and every pending sleep operation has ended.
Changing the configured password account cannot release a pending request for
the previous account. An overlapping sleep request waits for the earlier UI to
exit before the controller blanks the displays and acknowledges the next UI.
`ExecStopPost` handles success, failure and cancellation alike.

UI, controller and authenticator failures never invoke console unlock. The
controller can restart on the still-locked VT. If preparation is interrupted,
the completion hook starts recovery and finishes the recorded acquisition
intent before offering password authentication. Successful authentication closes
the UI, records authorization, restores the original VT/SysRq setting and removes
the state. The controller stays available between cycles to avoid racing a new
sleep request against the previous service's shutdown. It never blindly unlocks
the VT while recovering a failed display.

Plasma's separate resume prompt is suppressed when this option is enabled; its
idle lock remains configured normally. Before changing consoles, the root
controller requests native locking for the configured account's exact Plasma
Wayland session. It records the logind session, leader, start time, D-Bus identity
and service owner, and checks that identity and the full Plasma session inventory
again before release. The native root-only Lock method's synchronous reply returns
a fresh opaque token after processing the request; an already-true lock flag is
insufficient. Release checks and consumes that token, so a delayed older request
cannot unlock a newer native lock. Each reacquisition also invalidates the root
PAM generation. One fresh Linux-password success releases both that native
lock and the root console lock, including when Plasma was already locked before
sleep. Another account, replacement session or changed bus cannot inherit that
authorization. Interrupted release requires a new password attempt and request
generation; the controller never replays an older successful authentication.

If KWin restarts inside that same logind session, the controller may record its
new D-Bus identity only after rechecking the unchanged session inventory, UID,
leader and start time. This changes the request generation, discards old PAM
results, and requires a newly acknowledged native lock before another password
prompt. A replacement logind session is refused. The opt-in compositor restart
patch must start KWin locked before reconnecting clients; root polling alone
cannot protect the first frames after a crash.

Native ownership is local to one compositor process. A replacement compositor
starts locked but does not inherit the previous root token. During the short
interval before the controller rebinds, a fresh successful native Linux-password
authentication can release its native lock. Old greeter authentication cannot
cross that process replacement. The physical console remains locked and still
requires fresh root PAM, so this crash-recovery case can require a second
password prompt. This is not a claim of continuous root-only native ownership
across compositor replacement.

While the root controller owns the native lock, native PAM completion and grace
paths cannot release it. Their greeter can restart while the compositor remains
locked; only the current root token authorizes release. Native password locking
continues normally outside that managed interval. This opt-in KScreenLocker
package ignores generic logind Unlock signals, including `loginctl unlock-session`:
those signals also accept the session UID and carry no request generation or
original caller credentials. There is no fallback to that interface. The three
root-control methods verify the D-Bus sender's actual UID and reject non-root
callers; no authentication agent is invoked. The opt-in session-bus policy admits
UID 0 alongside the normal bus owner, so these calls retain their real root
identity. Its mandatory connection rule allows only `user="root"`, with no user
wildcard. The pinned [broker v37 owner fallback](https://github.com/bus1/dbus-broker/blob/v37/src/launch/policy.c#L223)
remains intact. This does not admit other user accounts or weaken the native UID
check. Activation requires a fresh user bus or successfully reloaded policy;
do not restart a live user bus ad hoc.

Plasma X11 is explicitly refused during capture, supervision and release because
the compositor boundary is scoped to Wayland. Non-Plasma sessions, including
Gamescope, retain the physical console path and their own rollout gates. This
option does not cover an explicit
Gaming Mode idle/manual-lock policy, remove encryption keys from suspended RAM,
or protect against an already-compromised account or root process.

## VM coverage

The final production code passed these graph scenarios on 2026-11-07. Capture
and multioutput used candidate `7cbe931`; authentication and Sunshine used
`f67f90b`, whose runtime production files and relevant fixtures are identical.
The newer shared client fixture is covered by multioutput run `45520e4194d4`.
Each result has a completed driver cleanup and a valid Nix test output.

| Scenario | Graph run | Test duration | Cleanup complete |
| --- | --- | --- | --- |
| Authentication, failure recovery and s2idle | `6b51253c1f40` | 357.08 s | 357.53 s |
| PipeWire capture and compositor crash recovery | `b62de0af0fe8` | 435.30 s | 435.70 s |
| Two DRM cards, faults and s2idle | `45520e4194d4` | 161.13 s | 161.58 s |
| Sunshine/Moonlight stream and locked reconnect | `a70a92529450` | 201.44 s | 202.04 s |
| Existing boot touch unlock | `5ee9e7444cb5` | 75.43 s | 75.54 s |
| Existing rotated boot touch unlock | `8ae522bc5eec` | 76.59 s | 76.70 s |

Both boot tests used the byte-identical Plymouth `0014` readiness patch present
in the final candidate. Their boot configuration and fixtures are unchanged.
These regressions establish VM compatibility, not physical OLED touch mapping.

Run through the repository graph, from a built checkout:

```sh
make build
nix-shell tests/shell.nix --run 'NIXOS_CONFIG_ROOT=$PWD ./build/nixosctl graph tests/graph.json run sleep-lock-authentication'
```

The scenario boots real Plasma/SDDM, opens a working Konsole session, and uses
actual PAM, Plymouth, libinput touch events and emulated physical keyboard input.
Its public fixture password is `a`. It checks wrong passwords, successful touch
and keyboard authentication, VT/session-switch attempts, service/display/native
process failures, interrupted preparation before and after console acquisition,
unprivileged IPC denial, overlapping requests, cancelled sleep, unavailable lock service,
expired-account rejection, private state and absence of a distinctive password
from the guest journal. The updated scenario also registers a real logind X11
session to check refusal, and interrupts release after native unlock to require
one fresh password for a later native lock. It also changes the configured PAM
account during a pending request and requires refusal before another
authenticator starts. Authentication run `6b51253c1f40` passed these cases, denied
all three root-control methods to the session UID, and confirmed that generic
logind Unlock cannot release the managed native lock. It includes the final
initial-inactive input fix and completed the entire scenario after the earlier
deadline-interrupted attempt.
Injected sleep failures exercise ordering deterministically; a separate case
invokes the real hibernation helper without swap. The final case requests sleep
through logind and exercises two QEMU s2idle suspend/wake cycles through ACPI
power-button events. The second cycle starts while the password prompt is open;
input from the earlier request must be discarded. After authenticating the new
request, the original terminal must render and accept keyboard input again.

An unlocked control uses the same virtual hardware and actual sleep path. QEMU
RTC wake failed in both locked and unlocked controls; these tests use a simulated
ACPI power button and wait for actual suspend-to-idle before waking. The fixture
ignores the button as a shutdown command. This is not a hardware RTC-wake claim.
The virtual display uses Linux 7.3's virtio-gpu suspend callbacks and QEMU's
`x-pcie-pm-no-soft-reset=on`; older fixture combinations lost their display even
with the locker disabled. The control checks actual visible rendering and input,
not merely that the desktop process remains alive.

The guest uses a temporary EROFS store image without virtio-fs devices because
virtio-fs rejects suspend. Test cleanup removes the guest disks; retained output
contains logs/screenshots. This test does not use `--keep-failed`.

The `sleep-lock-multioutput` graph (`45520e4194d4`) passed with two separate DRM cards and
two private fullscreen windows. It verifies visible concealment, touch unlock,
live rendering and input restoration after an actual VM suspend/resume cycle,
and recovery from injected open, map, master and modeset failures on the second
card. Each failed acquisition must deny sleep and remove private pixels from
both scanouts before retrying. See
[the multioutput fixture](../../../../tests/sleep-lock/multioutput/README.md)
for its physical-hotplug limitations. This final run includes the updated shared
client; the earlier `135accc2a798` pass does not replace that coverage.

## Compositor restart validation

The failed capture runs `986266db3b8e` and `674673c2fa94` exposed separate
inactive-restart defects and fixture problems. `kwin-resume-paused-device.patch`
retains logind's paused DRM handle so the existing ResumeDevice callback can
initialize the physical output after logind restores DRM master ownership.
It also defers rendering-device creation while that handle is not master,
including when video-group permissions allow an early direct node open.
Normal and video-group restart controls passed two cycles with this correction.

The subsequent `kwin-suspend-inactive-input.patch` applies the initial inactive
state to libinput. Subscription and state snapshot happen on the session thread;
input setup is queued before later transitions. This ensures activation resumes
libinput and reopens revoked device descriptors. The old-package diagnostic
`95872e5c8382` restored physical and virtual private pixels but retained zero
keyboard devices and failed fresh keyboard input. It does not validate the new
input patch.

Focused control `f054e50a8fce` then passed with the new input patch and the same
KWin output as the final suites (`wv1289zpdsxkjvjxj5zjyxammggqxv05`). It restored
physical and virtual private pixels, three keyboard devices, and fresh QMP
keyboard input after the inactive restart; cleanup completed at 74.49 seconds.
This establishes the focused regression fix; the full capture scenario below
separately validates the production sleep-lock path.

The capture fixture separately keeps its supported udmabuf software allocator
available to the guest observer while inactive. Otherwise Mesa's software
render-node fallback issues CREATE_DUMB on a node where that ioctl is forbidden.
The fixture also places its windows on their intended outputs through KWin's
scripting API after hotplug, then activates the physical test window before
checking fresh input. These are guest fixture changes, not production device
permission changes. See the [capture scenario](../../../../tests/sleep-lock/capture/README.md)
for the distinct failure evidence and assertion scope.

Full capture run `b62de0af0fe8` subsequently passed the complete production
root-lock scenario, including concealment during compositor restart, refusal of
stale and incorrect authentication, and restoration of physical output, virtual
capture and fresh keyboard input after one correct root password. Authentication
`6b51253c1f40` also completed its full rerun. Their earlier deadline interruptions
are historical attempts, not the final result.

## Remaining rollout gates

The direct DRM [Gamescope fixtures](../../../../tests/sleep-lock/gamescope-native/README.md)
passed console rendering/input and the non-PM root-lock flow. Real suspend
failed with the lock enabled and in an otherwise identical unlocked control:
the native-context virtual GPU lost its resources/scanout. That identifies a VM
power-management limitation, not Steam Deck hardware success or failure.

Before host enablement: test Gamescope/Gaming Mode, session transitions, all
connected outputs, the OLED's actual rotation/touch mapping, physical keyboards,
NVIDIA/AMD handoff, and real suspend/hibernation success and failure. VM s2idle is
not proof of Steam Deck s2idle or hibernation reliability. Privately verify the
intended Linux password, preserve authenticated administration and a known-booted
fallback, and provision the required Sunshine credentials before staged
activation. The public VM password proves only the fixture's PAM flow; it does
not verify either physical account. Missing Sunshine provisioning can prevent
streaming after activation.

The DRM checks do not cover compositor virtual outputs or remote capture.
Native Plasma locking provides the intended boundary for KWin virtual outputs.
The final PipeWire capture scenario verifies actual frames through greeter
failure, pending authentication and compositor replacement; its result is not
inferred from a locked D-Bus state. Actual Sunshine/Moonlight run `a70a92529450`
passed existing-stream concealment, remote wrong/correct-password input and
reconnect by the same paired client while locked. See the
[Sunshine test scope](../../../../tests/sleep-lock/sunshine/README.md). They cover
SDR software encoding and an existing virtual-output creator, with sampled
decoded frames. They do not prove hardware encoding, Gamescope capture, real
suspend, production virtual-display lifecycle, or every queued remote frame.
The remote view shows Plasma's lock screen while input reaches root Plymouth;
matching remote prompt/error presentation remains an acceptance item.
