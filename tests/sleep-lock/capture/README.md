# Plasma virtual-output sleep locking

Full capture run `b62de0af0fe8` **passed** on 2026-11-07 with candidate `7cbe931`:
435.30 seconds, cleanup complete at 435.70 seconds, and a valid Nix test output.
It completed all assertions, including physical rendering, virtual capture and
fresh keyboard input after compositor crash recovery. The feature remains
disabled by default pending physical-host acceptance.

Run through `make build` followed by the `sleep-lock-capture` graph node. The
fixture uses a real Plasma Wayland session, KWin's restricted screencast
protocol, a software virtual output, PipeWire, and a separate Qt client with
recognizable private pixels. Only the probe's desktop entry is granted the
screencast interface. The test does not disable KWin's protocol filtering.

The probe keeps its stream alive across native lock, VT handoff, wrong root
passwords, Plymouth crashes, and failed native-greeter execution. It records
actual RGB frames and a sequence/timestamp/private-pixel count for every
received frame. A snapshot taken immediately after production sleep prepare
must already conceal the private client. After fresh root PAM succeeds, the
same client becomes visible and receives real keyboard input again, including
when the native session was already locked before sleep.

The native control uses real wrong and correct passwords through Plasma PAM.
Separate root-token cases reject every privileged method from the session UID,
reject an older token after a new acquisition, and ignore the session owner's
unversioned logind unlock even after an ordinary Lock invalidates the token.
A stopped greeter holds an authorized termination pending while a newer root
lock supersedes it; its eventual exit must not reveal private pixels. A fresh
successful native PAM attempt while root ownership is held must also leave
the compositor locked, independently of the root supervisor.

A fixture-only PAM rule delays native authentication **after** `pam_unix`
accepts the password. The test requests sleep through the production boundary,
pauses only the VM's root supervisor after acknowledgement, releases the old
native result, and checks the entire frame history. This prevents a fast
supervisor relock from hiding a transient leak. Retiring the old greeter must
keep the native compositor locked independently of that poll.

The compositor-crash case checks the wrapper's actual `--lockscreen` restart,
reconnects a new virtual output and private client, and checks that the same
logind session receives a new root authentication epoch. Buffered old input and
a wrong password cannot release it. The final assertion requires one fresh
correct root password to restore the physical output, capture and input; that
assertion passed with the final patches. While VT63 owns DRM, the restarted
compositor is expected to expose only the virtual output. A crash now requires
a password even if it happened while the desktop was previously unlocked.
The reconnected private client explicitly enables its fixture-only hotplug
mode to paint the virtual output immediately and the physical output when it
returns; the independent multioutput scenario still requires two screens.

## Inactive-restart diagnosis and fixture prerequisites

Earlier failed capture runs `986266db3b8e` and `674673c2fa94` exposed distinct DRM,
input-initialization and fixture problems. They kept capture concealed and
accepted fresh root PAM, but did not restore usable physical rendering and
input. Those failures motivated the corrections validated by the final run.

The paused-device lifetime bug also reproduced without the root controller,
PAM, Plymouth or `VT_LOCKSWITCH`, including with unmodified KWin. When a second
open failed while inactive, KWin discarded the acquired logind handle and its
future ResumeDevice notification. The production
`kwin-resume-paused-device.patch` retains that handle before attempting the
rendering-device open and waits until logind restores DRM master ownership.
The existing resume callback can then retry initialization. Both normal and
video-group controls passed two inactive-restart cycles with the patch; early
direct node access does not permit initialization from a paused, non-master
handle.

The later capture failure had both outputs present but persistent
`DRM_IOCTL_MODE_CREATE_DUMB failed: Permission denied`. The VM's active-session
udmabuf ACL disappears while another VT is active. A restarted KWin then chooses
Mesa's software renderer on `/dev/dri/renderD128`, whose CREATE_DUMB allocation
is rejected because the ioctl is not allowed on render nodes. The fixture's
`softwareRenderAccess` parameter defaults to true and gives its guest observer
persistent render-group access to `/dev/udmabuf`; it changes no production or
host device permissions. The false variant preserves the original prerequisites
for diagnosis.

After physical output hotplug, Qt's client-side screen selection alone also
left private fixture windows on the virtual output. Their captions now identify
the intended screen, and `sleep-capture-place` uses KWin's supported
`sendClientToScreen` API after output configuration. It runs only initially
unlocked and after successful root PAM. The helper activates the physical
fixture window through the writable `workspace.activeWindow` property so fresh
keyboard input has a deterministic destination. Private-pixel and real-input
assertions remain mandatory; final physical OCR and input waits are bounded to
60 and 30 seconds respectively.

Old-package diagnostic `95872e5c8382` independently passed physical PRIVATE
DISPLAY OCR and virtual private-pixel checks after the allocator and placement
fixes, but fresh keyboard input still failed. `ListKeyboards` changed from three
devices before restart to zero after restoration, and KWin held no input-event
descriptors. Its libinput connection subscribed to later session transitions
without applying the initial inactive state, so activation could skip resume.
The production `kwin-suspend-inactive-input.patch` subscribes and snapshots on
the session thread, then queues initial input setup before later transitions;
an initially inactive context is suspended so activation reopens its devices.
This diagnostic predates that correction and is not its positive validation.
The final capture `b62de0af0fe8`, authentication `6b51253c1f40`, multioutput
`45520e4194d4` and Sunshine `a70a92529450` runs all passed with the same production
code. The authentication and Sunshine checkouts at `f67f90b` have byte-identical
runtime code and relevant fixtures to candidate `7cbe931`; the final multioutput
run covers the newer shared client fixture.

This scenario exercises the pinned Plasma **Wayland** full-output capture path used by
Sunshine's KWin backend. Separate [actual Sunshine/Moonlight tests](../sunshine/README.md)
passed existing-stream concealment, remote password/input recovery and
same-client reconnect while locked in final run `a70a92529450` (201.44 seconds,
cleanup complete at 202.04 seconds). These cover SDR software encoding and
sampled decoded frames while the existing virtual-output creator remains alive;
they complement this suite's complete PipeWire frame history.
This scenario does not claim general protection from malicious
processes running as the logged-in user or direct window/thumbnails capture.
`GetActive` and `LockedHint` acknowledge compositor state, not remote encoder
queues or frames already delivered to another machine. Remote input/unlock UX,
physical AMD/NVIDIA output combinations, Gamescope, display hotplug, and the
Steam Deck's actual suspend mode remain separate hardware acceptance gates.
The [direct DRM Gamescope VM fixtures](../gamescope-native/README.md) passed the
non-PM lock flow, but real suspend lost scanout/input in both locked and unlocked
native-context controls. That virtual GPU limitation is not physical Steam Deck
validation and does not waive its suspend gate.
The feature stays disabled by default.

Focused control `f054e50a8fce` passed on 2026-11-07 with the final input-patched
KWin output `wv1289zpdsxkjvjxj5zjyxammggqxv05`. After inactive restart it restored
physical PRIVATE pixels, virtual capture, all three keyboard devices, and the
actual QMP keyboard marker AFTERRENDER. Cleanup completed at 74.49 seconds. The
old-input control `95872e5c8382` failed this input boundary with zero keyboards.
The full `b62de0af0fe8` pass separately covers the production root-lock scenario.
Its earlier deadline-interrupted attempt is historical; the resumed run
completed the whole test and cleanup.

The remote view shows Plasma's native lock screen while remote password input
reaches the root Plymouth prompt on the physical console. It does not mirror
Plymouth's prompt or failed-password feedback. Compositor ownership is local to
one process: a replacement starts locked, but a fresh native Linux-password
authentication before root rebind can release that native lock. The physical
root lock still requires fresh root PAM, potentially producing a second prompt.
Neither continuous root-only ownership across replacement nor matching remote
prompt presentation is claimed. Physical rollout also requires private
verification of the intended Linux password and required Sunshine provisioning.
