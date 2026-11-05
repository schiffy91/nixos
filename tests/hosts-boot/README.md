# Booting the real hosts

`tests/hosts-boot/test.json` installs and boots a host's **own** configuration in
a VM. The fixtures under `tests/hosts/` wrap the real host module and may add
only VM plumbing — a serial console, the disk interface, initrd modules for the
virtual GPU and keyboard, a test key and password, and a health reporter. They
never turn a feature off to make a test pass. `VM-TEST`, used by the installer
tests, does the opposite: it disables plymouth, Home Manager and the app set,
which is why none of the failures below were ever caught there.

Nodes: `host-boot-{STEAM-DECK-OLED,FRACTAL-NORTH}-VM-{standard,secure}`.

## What each run asserts

| Goal | Assertion |
| --- | --- |
| Offline install | outbound TCP and DNS are rejected before `nixosctl install`; the log must show the baked closure was used, and must not show a disko fetch, a build, or a download |
| Install leaves a resettable system | every reset volume has a published read-only `CLEAN`, with no leftover `CLEAN.tmp.*` |
| Unlock | the unlock agent the host declares actually ran (`unl0kr` or `plymouth`), and the passphrase is typed as USB key events, never over the serial line |
| Clean boot | `is-system-running` = `running`, zero failed units, zero units systemd refused, `home-manager-<user>.service` active |
| Quiet boot | `quiet` and `loglevel=3` are in effect on `/proc/cmdline`, and the panel capture is not a text console |
| Secure Boot | the fallback loader, systemd-boot and the UKI are signed; no Microsoft key is enrolled; where the host enrols its own keys, the firmware reports `enabled` |
| Hibernate | the system hibernates once per installation, resumes behind the passphrase, and reports `E2E-RESUMED` |
| Immutability | a `CLEAN` snapshot survives every boot, and swap is active |
| Stability | the health fields of the third boot are identical to the first |

The guest reports over the serial console because the installed hosts authorize
the admin's own keys, not the harness's, so there is no SSH into them.

## Not covered here, and why

A VM is not a Steam Deck. These are untested by this suite and must be checked
on hardware:

- **The panel itself** — brightness, the OLED, and unl0kr's portrait rotation.
  We capture the virtio-gpu framebuffer, not the Deck's screen.
- **The digitizer** — the passphrase arrives as USB HID key events through QMP.
  Touch input to unl0kr's on-screen keyboard is never exercised.
- **A real GPU** — Gaming Mode, gamescope, Steam, amdgpu, Mesa, HDR and VRR do
  not render in a VM. The session reaching its target is therefore not asserted.
- **Plymouth's splash as pixels** — a plymouth splash is almost entirely black,
  and so is a text console, so the pixel check runs only for unl0kr, whose
  on-screen keyboard is unmistakable. On plymouth hosts, ownership is taken
  from the journal instead.
- **Console silence on the physical panel** — the fixtures deliberately route
  the kernel console to `ttyS0` so the harness can read it, so serial carries
  the log by design. Quietness is asserted from the kernel command line and the
  panel capture.
- **Secure Boot enforcement on FRACTAL-NORTH** — that host sets
  `autoEnrollKeys = false` on purpose, so enrolment is the documented manual
  `nixosctl secure-boot enable`, which performs a full rebuild. Driving that
  from the fixture makes a run take hours, and it is already covered end to end
  by `secure-boot-install` and `secure-boot-lanzaboote`, which enrol keys and
  assert `bootctl status` and `sbctl verify`. This node therefore asserts a
  clean boot and that no vendor keys are enrolled, not firmware enforcement.
  The Deck does enrol automatically, so its node asserts enforcement in full.
- **Firmware updates under Secure Boot** — `jupiter-biosupdate` and the Jovian
  BIOS/controller updates are Microsoft-signed and cannot be exercised here.
- **TPM2** — the Deck deliberately has no TPM2 keyslot.
- **Radios and controllers** — Wi-Fi, Bluetooth and `hid-steam`.
- **s2idle** — only hibernation (S4) is exercised; suspend-to-idle is hardware.

`install-offline` is a fast smoke test of the same offline install against the
QEMU host. It stops once the install is verified: that host grants the harness
no SSH either, and booting an installed system is covered here instead.
