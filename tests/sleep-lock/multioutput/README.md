# Multiple DRM cards and outputs

Run through `build/nixosctl graph tests/graph.json run sleep-lock-multioutput`
after `make build`.

Final run `45520e4194d4` passed on 2026-11-07 with candidate `7cbe931`, including
the updated shared client fixture: 161.13 seconds, cleanup complete at
161.58 seconds, and a valid Nix test output. This supersedes the earlier
`135accc2a798` result for that fixture. Sleep locking remains disabled by default;
passing these virtual displays does not enable or certify either physical host.

This extends the real Plasma Wayland authentication fixture with two distinct
virtio DRM cards, each with its own scanout. A Qt client renders different private
content fullscreen on each output and accepts real keyboard input. The test
checks both captured scanouts rather than inferring coverage from a running
daemon or active VT.

It verifies that the sleep lock replaces both private frames with its password
prompt, rejects an incorrect password without delivering input to the client,
covers both outputs after a Plymouth crash, accepts touch authentication on the
explicitly mapped touchscreen, and restores both live windows and input after
authentication. A request typed into the client invokes `systemctl suspend` as
the active desktop user. The test then waits for actual s2idle, wakes through a
QEMU ACPI power-button event, and repeats the coverage/unlock/input checks.

The test then injects open, buffer-map, DRM-master and modeset failures into
Plymouth on the second card. A separate pre-blanking helper remains unmodified.
Both scanouts are sampled throughout each rejected request; private client
colors must be absent, the sleep backend and PAM must not start, and forced
session activation must retain VT63. Clearing each fault must restore the
password UI without unlocking first. Physical hotplug remains a separate gate:
QEMU rejects hotplugging the fixture's virtio-gpu-pci device, so the attempted
third-card hotplug scenario could not exercise that behavior.

The synthetic touchscreen is associated with `card0-Virtual-1` using the existing
`WL_OUTPUT` udev contract because both cards name their connector `Virtual-1`.
Without that association, Plymouth intentionally shows the password prompt on
both outputs but declines to guess which one should receive touchscreen input.

The VM uses Linux 7.3, QEMU's `x-pcie-pm-no-soft-reset=on`, and harmless Shift
input after resume, matching the passing unlocked suspend control. Their
individual contributions to avoiding the virtual-GPU resume failure have not
been isolated. They do not establish physical AMD/NVIDIA, Steam Deck, or Gaming
Mode behavior.
Actual Gamescope suspend remains a separate rollout gate. The
[direct DRM Gamescope fixtures](../gamescope-native/README.md) passed console
rendering/input and the non-PM root-lock flow, but real suspend lost the native
virtual GPU's resources/scanout in both locked and unlocked controls. This is a
VM limitation, not evidence of Steam Deck suspend success or failure.

The stricter readiness contract checks actual DRM ownership, framebuffer IDs
and plane bindings. These injected failures do not prove safety when a physical
driver refuses pre-blanking itself, or eliminate every race during physical
hotplug, dock changes or resume. Such failures deny sleep; they do not justify
claiming that an uncooperative display has been blanked.
