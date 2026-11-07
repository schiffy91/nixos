# Sunshine sleep-lock streaming test

Run `sleep-lock-sunshine` through the repository graph after `make build`.
The node is required and uses two disposable VMs, each with two CPUs and 4 GiB
of RAM. It requires ordinary x86 KVM support, not a host GPU or render-node
permissions. The host operation is bounded to 30 minutes, including builds;
Nix builds use one job and four cores. The driver removes guest disks on exit.

The server runs the sleep-lock Plasma packages, actual Sunshine with the
repository credential guard, KWin capture of an existing virtual monitor, and
software H.264 encoding. A separate X11 guest runs the real Moonlight client
with software decoding. Pairing and administration use disposable public test
credentials; no production account, vault, SSH agent or host session is used.

Before locking, the client must display an updating private application and
deliver a keyboard marker through Moonlight and Sunshine's libvirtualhid input.
A deliberately failed suspend backend must leave the root password boundary
active. The decoded display must change to a concealed frame while the private
application continues updating. Subsequent sampled decoded frames must remain
concealed. A wrong password typed through Moonlight must reach the root password
prompt, fail authentication, and never reach the private application. A fresh
correct Linux password through that same client must restore both streaming and
application keyboard input. The fixture password is `a`.

The test locks again, terminates the active Moonlight stream, and verifies that
the same root lock remains held. The already paired client then reconnects.
Sunshine must create a new KWin/PipeWire capture and deliver decoded native-lock
pixels before any correct password is entered. A wrong password must remain
isolated from the live private application; the correct Linux password must
restore video and application input through the new stream.

Run `4e742757c9c9` passed on 2026-11-07 in 121.81 seconds, followed by successful
driver cleanup. The client screenshots show private frame 108 before locking,
Plasma's native lock screen while locked, and private frame 178 after release.
Run `a50524b0281d` also passed the reconnect case in 176.76 seconds, followed by
successful cleanup of both guests. `reconnected-locked.png` shows the native
lock screen before authentication; `reconnected-unlocked.png` shows private
frame 306 after correct authentication and restored application input.

This is SDR coverage of an existing stream across a failed sleep operation and
fresh capture creation by the same paired client while locked. The virtual
output's creator stays alive throughout; this does not exercise production
virtual-display creation/destruction or a new client's identity/pairing.
Pixel checks sample decoded screenshots; they are not an exhaustive recording
of every encoded frame. The separate capture suite records KWin/PipeWire frame
history. This test does not cover hardware encoders, HDR, actual suspend or
network loss, gamepad input, Gamescope/KMS,
or physical display/touch behavior. It intentionally shares the sleep fixture's
KWin build rather than adding the production HDR backport and rebuilding KDE.

The remote view shows Plasma's native lock screen while password input goes to
the root Plymouth prompt on the physical console. Remote password entry works,
but that view does not mirror Plymouth's prompt or failed-password feedback;
remote unlock presentation remains an acceptance item.

Final production combination `f67f90b` also passed run `a70a92529450` on
2026-11-07: 201.44 seconds, cleanup 202.04 seconds. Both existing and reconnected
streams passed the complete assertions. The original graph wrapper timed out,
but its detached Nix builder completed and the second bounded graph client
recorded PASS against the same immutable derivation. No guest or owned client
remained afterward.
