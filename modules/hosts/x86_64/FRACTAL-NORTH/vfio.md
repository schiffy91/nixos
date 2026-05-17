# VFIO + Gaming VM on FRACTAL-NORTH

A plan for passing the RTX 4090 to a Windows gaming VM while the host
continues to drive the Pro Display XDR from the AMD iGPU — with HDR,
full refresh rate, and disks living on the Thunderbolt 4 NVMe.

This is a working document, not a how-to. Validate each phase before
moving on, because some of the choices have one-way consequences for
the host's desktop session.

---

## 1. Goal

Boot the host normally → daily-drive on the iGPU → on demand, "switch
to VM mode", which:

1. Releases the NVIDIA 4090 from KWin / PRIME / any process that has
   it open.
2. Re-binds `01:00.0` + `01:00.1` (GPU + HDMI audio) to `vfio-pci`.
3. Launches a Windows 11 VM whose disk lives on the TB4 SN850X.
4. The VM's video output drives a physical monitor wired to the 4090,
   at native resolution / refresh / HDR.
5. On VM exit (or by hotkey), rebinds the GPU back to `nvidia` and
   restores the host session — without rebooting.

Stretch goal: persist this across NixOS rebuilds so it survives the
immutable-root reset.

---

## 2. What the hardware actually looks like

Probed live from sysfs, not assumed:

| PCI addr      | IOMMU grp | Device                   | Driver         | Notes                              |
|---------------|-----------|--------------------------|----------------|------------------------------------|
| `0000:01:00.0`| 12        | NVIDIA RTX 4090 [10de:2684] | `nvidia`     | Passthrough target                 |
| `0000:01:00.1`| 12        | NVIDIA HDMI audio [10de:22ba] | `snd_hda_intel` | In same group → must pass too |
| `0000:6b:00.0`| 19        | AMD Raphael iGPU [1002:164e] | `amdgpu`    | Host scanout, stays put            |
| `0000:6b:00.1`| 20        | AMD audio                |                | Host audio, stays                  |

Group 12 contains **only** the two NVIDIA functions — clean isolation,
no ACS override needed.

Display wiring (read from `/sys/class/drm/card*-*/device`):

| Connector    | Wired to | Status       | What's on it          |
|--------------|----------|--------------|-----------------------|
| `card1-DP-1` | AMD      | connected    | Pro Display XDR       |
| `card1-DP-2` | AMD      | connected    | (second host display) |
| `card0-DP-3` | NVIDIA   | connected    | Sunshine dummy (EDID firmware) |
| `card0-HDMI-A-2` | NVIDIA | connected  | Samsung S89C          |
| `card0-DP-4/5`   | NVIDIA | disconnected | free outputs          |

**Critical implication**: when the 4090 is passed to the VM, the host
loses DP-3 (don't care — Sunshine dummy) and HDMI-A-2 (S89C). The Pro
Display XDR stays alive on AMD. So no SSH/recovery dance is needed
for the host to stay usable — KWin keeps rendering on the iGPU.

This is the single most important architectural fact for this plan. It
also rules out single-GPU passthrough mode in vm-curator: we are
firmly in **multi-GPU passthrough** territory.

Current kernel cmdline already has `iommu=pt` (good). Missing:
explicit `amd_iommu=on` (works without it but worth making explicit),
and the VFIO modules in `boot.kernelModules`.

---

## 3. Why vm-curator is right but needs adaptation

[mroboff/vm-curator](https://github.com/mroboff/vm-curator) is a Rust
TUI over QEMU/KVM. Relevant for us:

- It generates `launch.sh` per VM, with sane defaults for OVMF, TPM,
  q35, virtio-* devices.
- It has a **multi-GPU passthrough** mode: extra `-device vfio-pci`
  args plus a bind/unbind dance via `pkexec`/`sudo` at launch time
  (`Fix Multi-GPU Passthrough VFIO Binding` in v0.4.6 changelog).
- It enumerates PCI by sysfs and computes IOMMU groups itself
  ([src/hardware/pci.rs](https://github.com/mroboff/vm-curator/blob/main/src/hardware/pci.rs)).
- It already ships a Nix flake. Adding it as a flake input and
  exposing the binary is one-line work.
- It writes VM dirs to `~/vm-space/<vm-name>/` (containing
  `launch.sh`, `disk.qcow2`, `single-gpu-config.toml`, OVMF vars).

What does **not** work on NixOS:

- vm-curator's "System Setup" wizard writes `/etc/modules-load.d/vfio.conf`
  and `/etc/modprobe.d/vfio.conf` and runs `mkinitcpio -P` /
  `update-initramfs -u`. None of this survives a NixOS rebuild and the
  `/etc` writes will be blown away by the immutable root reset
  ([modules/system/immutability.nix](../system/immutability.nix)). We
  have to do that part in Nix.
- The bind/unbind script assumes the user is happy to be prompted by
  polkit on each launch. We can do better with a dedicated helper.
- The default VM directory (`~/vm-space/`) is *not* in the immutability
  persist list ([modules/settings.nix](../../settings.nix#L65)). Put
  VMs on the TB4 NVMe instead and symlink.

So: take vm-curator as the **interactive front door** (VM CRUD,
launch, snapshot, USB picker), but own the system-level VFIO config
and the GPU handoff ourselves through Nix + the existing `nixos`
Python CLI.

---

## 4. Architecture

```
┌──────────────────────────────────────────────────────────────────┐
│ Boot                                                             │
│   kernelParams: amd_iommu=on iommu=pt                            │
│   kernelModules: vfio vfio_iommu_type1 vfio_pci  (load early)    │
│   NVIDIA stays bound to nvidia driver → PRIME offload works      │
│   normally, daily-driver mode                                    │
└──────────────────────────────────────────────────────────────────┘
                            │
                            ▼  user invokes `nixos vfio enter`
┌──────────────────────────────────────────────────────────────────┐
│ VM mode                                                          │
│   1. Disable DP-3 + HDMI-A-2 in KWin (via kscreen-doctor)        │
│   2. Tell KWin to only use amd-card                              │
│      (KWIN_DRM_DEVICES override; restart kwin_wayland)           │
│   3. Kill processes holding /dev/nvidia* (Steam, Chrome, etc.)   │
│   4. modprobe -r nvidia_drm nvidia_modeset nvidia_uvm nvidia     │
│   5. driverctl set-override 0000:01:00.0 vfio-pci                │
│      driverctl set-override 0000:01:00.1 vfio-pci                │
│   6. Launch vm-curator's generated launch.sh                     │
│      QEMU passes 01:00.0 + 01:00.1 + Bolt USB receiver           │
│      virtio-blk disk file lives on TB4 NVMe                      │
│   7. Monitor on HDMI-A-2 input-switches to the GPU's HDMI port,  │
│      Windows renders native (HDR enabled in Windows settings)    │
└──────────────────────────────────────────────────────────────────┘
                            │
                            ▼  VM shuts down (or `nixos vfio leave`)
┌──────────────────────────────────────────────────────────────────┐
│ Restore                                                          │
│   1. driverctl unset-override 01:00.0 / 01:00.1                  │
│   2. modprobe nvidia (modeset, drm, uvm)                         │
│   3. Restore KWIN_DRM_DEVICES, restart kwin_wayland              │
│   4. Re-enable DP-3 + HDMI-A-2                                   │
└──────────────────────────────────────────────────────────────────┘
```

The four-color decisions are noted inline below in §6.

---

## 5. Display + HDR strategy

There are three ways to get a window from a passthrough VM onto a
host monitor. Each has a clear use case:

### Option A — physical output on the GPU (chosen default)

Plug a DisplayPort cable from the 4090 into the monitor. When in VM
mode, press the monitor's input-source button. Windows sees a real
RTX 4090, enables HDR via DP HBR3, runs the panel at native Hz.

Latency: zero (it's a real cable). HDR: full DisplayPort HDR-10. Hz:
whatever the panel supports.

This is what we want for actual gaming. The S89C on HDMI-A-2 is
already wired to the 4090, so this works out of the box for that
panel. The Pro Display XDR is on the iGPU and stays the host display.

### Option B — Looking Glass (future)

Useful if you ever want the VM in a window on the host without
switching monitors. Adds an IVSHMEM shared framebuffer; the VM
renders to RAM, the host displays it. HDR support is preview-grade
and adds latency vs. native — not the right default for HDR gaming.

Defer. The plan keeps the door open (ivshmem can be added to the
QEMU args without re-architecting).

### Option C — Sunshine over loopback

Already running. Not for low-latency local use; ignore for this
flow.

Decision: **A** is the primary path. Document B as a follow-up.

---

## 6. Concrete work, broken into phases

Each phase ends in a working, revertable state. Don't skip the
verification at the end of each.

### Phase 0 — verify host preconditions (no changes)

```bash
# 1. AMD-Vi is alive and groups look right.
find /sys/kernel/iommu_groups/ -type l | grep -E "0000:01:|0000:6b:" | sort
# Expect: 01:00.0 and 01:00.1 in the same group, alone.

# 2. nvidia is currently bound, not vfio-pci.
lspci -nnk -d 10de: -s 01:

# 3. AMD iGPU is the host scanout for at least one connected display.
ls -l /sys/class/drm/card1-DP-1/device
# Expect: link to 0000:6b:00.0

# 4. CPU is AMD-V capable (almost certainly yes on Raphael).
grep -m1 -E 'svm|vmx' /proc/cpuinfo

# 5. SN850X is reachable, has space for a Windows image (60-200 GB).
df -h /path/to/tb4/mount
```

If any of these fail, the rest of the plan is moot.

### Phase 1 — system-level Nix changes

New file: `modules/hosts/x86_64/FRACTAL-NORTH/vfio.nix`, imported
from [FRACTAL-NORTH.nix](FRACTAL-NORTH.nix).

```nix
{ pkgs, host, ... }: {
  boot = {
    # iommu=pt is already in cpu.nix; add amd_iommu=on explicitly.
    kernelParams = [ "amd_iommu=on" ];
    # Load VFIO early so it can claim devices before nvidia would,
    # *if* we ever decide to pre-bind. Today we leave nvidia bound at
    # boot and rebind dynamically, but having vfio-pci available
    # eliminates an `insmod` step in the handoff script.
    kernelModules = [ "vfio" "vfio_iommu_type1" "vfio_pci" ];
    # NOTE: deliberately NOT setting boot.extraModprobeConfig with
    # `softdep nvidia pre: vfio-pci` and NOT setting
    # vfio-pci.ids=10de:2684,10de:22ba — that would stop nvidia
    # from binding at boot and kill PRIME offload for the host.
    # We want nvidia to own the GPU until we explicitly hand it over.
  };

  # driverctl persists driver_override across rebinds and is the
  # NixOS-blessed way to swap drivers on a PCI device. We use it
  # dynamically from the vfio-enter script — not at boot.
  environment.systemPackages = with pkgs; [
    driverctl
    qemu_kvm        # qemu-system-x86_64, qemu-img
    OVMF            # UEFI firmware for Windows guest
    swtpm           # TPM2 emulator (Win11 requires it)
    virt-manager    # libvirtd console + console viewer for debugging
    looking-glass-client  # parked for Option B
    inputs.vm-curator.packages.${pkgs.system}.default
  ];

  # libvirtd is optional — vm-curator runs raw QEMU. We install it
  # only for virt-manager's console (handy when the guest is headless
  # before drivers are installed).
  virtualisation.libvirtd = {
    enable = true;
    qemu = {
      package = pkgs.qemu_kvm;
      ovmf.enable = true;
      ovmf.packages = [ pkgs.OVMFFull.fd ];
      swtpm.enable = true;
    };
  };
  users.users.${config.settings.user.admin.username}.extraGroups =
    [ "libvirtd" "kvm" ];

  # Make the VM tree on the TB4 NVMe addressable by a stable path
  # and persist vm-curator's config across the immutable-root reset.
  # The actual fstab entry for the TB4 drive lives elsewhere; here
  # we only declare the directory exists and is owned by the user.
  systemd.tmpfiles.rules = [
    "d /mnt/tb4/vm-space 0755 ${user} users -"
    "L /home/${user}/vm-space - - - - /mnt/tb4/vm-space"
  ];
}
```

And in `flake.nix` add:
```nix
inputs.vm-curator = {
  url = "github:mroboff/vm-curator";
  inputs.nixpkgs.follows = "nixpkgs-unstable";
};
```

Persist `~/.config/vm-curator` by appending to the immutability
persist list in [modules/settings.nix](../../settings.nix#L65):

```nix
"/home/${config.settings.user.admin.username}/.config/vm-curator"
```

VM disks live under `/mnt/tb4/vm-space/<vm>/disk.qcow2`. The TB4
NVMe is mounted outside `/home`, so it bypasses the home subvolume
reset entirely — no extra persist entry needed for the disks.

**Verification**: `nixos-rebuild test`, then:
```bash
lsmod | grep vfio        # vfio_pci loaded
which vm-curator         # in PATH
driverctl list-devices | grep 0000:01:00  # both functions visible
```
NVIDIA should *still* be the in-use driver on `01:00.0`. PRIME
offload should still work (test with `nvidia-smi`).

### Phase 2 — the GPU handoff helper

New file: `scripts/bin/nixos/vfio.py`, wired into
[cli.py](../../../scripts/bin/nixos/cli.py) the same way `displays`
and `audio` are. Commands:

```
nixos vfio status   # show what's bound where + KWin display state
nixos vfio enter    # host → VM-ready  (idempotent)
nixos vfio leave    # VM-ready → host  (idempotent)
```

`enter` does, in order:

1. **Snapshot KWin state** to `/run/user/$UID/vfio-prev-displays.json`
   so `leave` can restore it. Use the existing
   [displays.py](../../../scripts/bin/nixos/displays.py)
   `Displays.list()` / `kscreen-doctor -o`.
2. **Disable NVIDIA-attached outputs** in KWin so KWin stops trying
   to scan out from the 4090:
   ```
   kscreen-doctor output.HDMI-A-2.disable output.DP-3.disable
   ```
3. **Stop processes holding `/dev/nvidia*`**. List them with
   `fuser -v /dev/nvidia*` and `lsof /dev/nvidia*`. The interesting
   long-running ones on this box: anything launched via
   [steam.nix](FRACTAL-NORTH/steam.nix)'s offload env, Chrome
   (configured for nvidia render node in
   [packages.nix](FRACTAL-NORTH/packages.nix)), Sunshine. We don't
   want to kill the user's session — instead, hand them a confirm
   prompt listing offenders, then SIGTERM only the listed PIDs.
4. **Unload nvidia modules** in the dependency order vm-curator uses
   ([src/vm/single_gpu_scripts.rs#L264](https://github.com/mroboff/vm-curator/blob/main/src/vm/single_gpu_scripts.rs#L264)):
   `nvidia_drm nvidia_modeset nvidia_uvm nvidia` (each with
   `modprobe -r`). Bail if any fails — usually means a process is
   still holding it; loop back to step 3.
5. **Rebind to vfio-pci**:
   ```
   driverctl --nosave set-override 0000:01:00.0 vfio-pci
   driverctl --nosave set-override 0000:01:00.1 vfio-pci
   ```
   `--nosave` is important: we do not want the override to persist
   across reboot. Default state is nvidia-bound.
6. Print a "ready" banner with the QEMU command the user should run
   (or just exec the chosen VM through `vm-curator launch <name>`).

`leave` is the reverse:

1. `driverctl --nosave unset-override 0000:01:00.0` (and `.1`).
2. `modprobe nvidia` then `nvidia_modeset`, `nvidia_drm`,
   `nvidia_uvm`.
3. Read back the snapshot and re-enable the disabled outputs.
4. The Sunshine `streaming-display-setup` user service in
   [sunshine.nix](FRACTAL-NORTH/sunshine.nix) is `wantedBy =
   graphical-session.target`, so DP-3 should auto-configure on
   re-enable. Verify and call it manually if not.

Implementation style: same as
[displays.py](../../../scripts/bin/nixos/displays.py) /
[audio.py](../../../scripts/bin/nixos/audio.py) — dense classmethods,
no docstrings, `from lib import Utils`. Add the matching `Vfio`
section in the `daemon.py` tray menu (
[daemon.py](../../../scripts/bin/nixos/daemon.py)).

**Verification**:
```bash
nixos vfio status     # baseline
nixos vfio enter      # GPU swaps drivers, S89C goes dark
lspci -nnk -s 01:00.0 # driver in use: vfio-pci
nixos vfio leave      # back to nvidia, S89C lights up
nvidia-smi            # works again
```

Run this loop a dozen times before *ever* attaching it to a VM. If
the rebind back to nvidia ever fails, the failure mode is "no display
on HDMI-A-2 / DP-3 until reboot" — annoying but not catastrophic
since the Pro Display XDR stays on the iGPU.

### Phase 3 — first VM

Create the Windows 11 VM through vm-curator's wizard, but point its
library at `/mnt/tb4/vm-space` (via the symlink set up in Phase 1).
Disk: 200 GB qcow2 on the TB4 SN850X. Memory: 32 GB (host has
plenty). CPUs: pin 8 cores via `-smp 8,sockets=1,cores=8,threads=1`
plus `taskset` if you want NUMA-style isolation later. UEFI + TPM
(Win11 requirement; OVMF + swtpm are already wired in Phase 1).

For the first install, **do not pass the GPU yet**. Use
`virtio-vga-gl` + spice so you can drive the installer from a host
window. Get Windows installed, drivers updated, RDP enabled.

Then snapshot (`vm-curator snapshot create pre-vfio`) and switch the
VM's launch config to multi-GPU mode (vm-curator → Management →
Multi-GPU Passthrough), selecting `0000:01:00.0` + `0000:01:00.1`.
That regenerates `launch.sh` with:

```
-device vfio-pci,host=0000:01:00.0,multifunction=on
-device vfio-pci,host=0000:01:00.1
-display none -vga none
-cpu host,kvm=off,hv_vendor_id=AuthenticAMD,hv_relaxed,hv_spinlocks=0x1fff,hv_vapic,hv_time
-machine q35,accel=kvm,kernel_irqchip=on
```

(Those flags are exactly what vm-curator generates for NVIDIA
passthrough — see
[src/vm/single_gpu_scripts.rs#L1160](https://github.com/mroboff/vm-curator/blob/main/src/vm/single_gpu_scripts.rs#L1160).
The `kvm=off + hv_vendor_id` mask hides the hypervisor from the
NVIDIA driver, which historically refused to load in VMs; modern
drivers no longer enforce this, but leaving the mask costs nothing.)

Add USB passthrough for the Bolt receiver. From
[FRACTAL-NORTH.nix](FRACTAL-NORTH.nix#L21):
- vendor `0x046d`, product `0xc548`.

vm-curator's USB picker handles this via the management menu. The
generated args will look like
`-device usb-host,vendorid=0x046d,productid=0xc548`. Plug a second
keyboard into a USB port that stays with the host.

**TB4 disk options**:
- *File on mounted FS* (chosen): `-drive
  file=/mnt/tb4/vm-space/win11/disk.qcow2,if=virtio,cache=none,aio=io_uring,discard=unmap`.
  Simple, snapshottable, plenty fast for gaming over TB4.
- *Whole-NVMe PCI passthrough*: tempting for max IOPS but the TB
  controller and downstream NVMe share an IOMMU group that pulls in
  bridges and other devices — would need an ACS override patch and
  is far more fragile, especially with `bolt` deauthorizing devices
  on suspend. Not worth it for this use case.

### Phase 4 — the actual session flow

End-to-end gaming session, what the user types:

```bash
nixos vfio enter      # ~5s, host loses HDMI-A-2/DP-3
vm-curator launch win11-gaming
# Press input-source on the S89C; Windows boots, HDR turns on,
# game runs at 240 Hz HDR
# ... play ...
# Shut down Windows from inside the guest
nixos vfio leave      # ~5s, host restored, can keep working
```

Or via the tray helper (Phase 5), one click.

### Phase 5 — tray integration

Add a "GPU" section to
[daemon.py](../../../scripts/bin/nixos/daemon.py) below "Displays",
showing the current driver binding and a one-shot
"Enter VM mode…" / "Leave VM mode" button. Use the same
`cli("vfio", "enter")` pattern as caffeine/audio.

The tray is the right place for it because the flow is "I'm done
with the host for a bit, hand the GPU over" — exactly the same shape
as caffeine inhibit. Keep it behind a confirmation dialog the first
time (lists processes that will be killed).

---

## 7. Things to decide before implementing

These should be answered before writing any Nix code, because each
flips a design choice:

1. **Auto-revert on VM exit?** Should `vm-curator launch` be wrapped
   so that the helper calls `nixos vfio leave` automatically when
   QEMU exits, or do we keep it manual? Manual is safer (gives a
   chance to inspect a hung guest) but is one extra step. Recommend:
   wrap it but expose a `--no-restore` flag.

2. **What to do with running NVIDIA-using processes?** The
   conservative choice is a confirm-with-list prompt. The fast
   choice is "always SIGTERM, restart on leave". The user-friendly
   choice is "Quit Chrome / Steam / Sunshine gracefully via their
   own protocols first, escalate to SIGTERM after 5s". Recommend:
   start conservative, add convenience later.

3. **Audio routing while in VM**: the 4090's HDMI audio goes with
   the GPU, so the S89C's speakers will be driven by Windows. The
   ProXDR / Kanto TUK setup on the host is unaffected. Nothing to
   do — call this out in the README.

4. **TB4 disk format**: qcow2 (chosen) gives snapshots and discard
   passthrough but ~5% perf hit vs raw. Raw is faster but loses
   snapshots. Default qcow2; switch to raw if benchmarks justify.

5. **CPU pinning**: not needed for Phase 3 (the Ryzen + 32 GB will
   handle this fine without pinning). Add it later only if there's
   measurable jitter; pinning across CCDs on Zen 4 needs care.

6. **Hugepages**: 1 GB hugepages give a small latency win and stable
   guest perf but eat host RAM unconditionally. Skip for v1.

---

## 8. Risks and how to back out

| Risk | Likelihood | Mitigation |
|------|------------|------------|
| `modprobe -r nvidia` hangs because something is still using it | Medium | Helper enumerates offenders and either kills or aborts; never blindly `-f`. |
| Driver swap leaves GPU in a wedged state | Low | `driverctl unset-override` + reboot always recovers. Pro Display XDR on the iGPU means the host is usable while debugging. |
| Win11 guest can't find OVMF firmware | Low | vm-curator's OVMF detection handles NixOS paths; the Nix module pins `OVMFFull.fd`. |
| TB4 link drops mid-game and the disk vanishes | Low-Medium | `bolt` keeps the drive authorized; mount with `nofail`; QEMU will pause the guest on I/O error rather than corrupt. Power the TB4 dock from a UPS. |
| Immutability reset wipes `~/.config/vm-curator` | High if not addressed | Added to persist list in Phase 1. Also: VM data lives on TB4, outside the reset entirely. |
| Booting with vfio claiming the GPU at boot (left a stale override) | Medium during dev | `--nosave` everywhere. A `vfio.py status` check in `displays`-style tray indicator surfaces the wrong state. Worst case: `driverctl unset-override` + reboot. |
| KWin doesn't re-pick up HDMI-A-2 after `leave` | Medium | The Sunshine streaming-display-setup unit already exercises kscreen-doctor on session start; reuse that idiom in the leave script. |
| Pre-commit / immutability hook bites the script's runtime state files | Low | Helper writes state to `/run/user/$UID` only — tmpfs, no persistence concern. |

---

## 9. Verification checklist (end-to-end)

Run this after Phase 5 is live. All must pass before declaring the
setup "done":

- [ ] Cold boot → desktop on ProXDR via iGPU, S89C lit via NVIDIA,
      `nvidia-smi` works, Steam launches with PRIME offload env vars.
- [ ] `nixos vfio status` shows `nvidia` for `01:00.0/.1`.
- [ ] `nixos vfio enter` succeeds, S89C goes dark, `lspci -k`
      reports `vfio-pci` for both functions, ProXDR stays on, host
      remains responsive on Wayland.
- [ ] `vm-curator launch win11-gaming` boots Windows; switching the
      S89C's input source shows the Windows desktop at native
      resolution and refresh; HDR toggle in Windows Settings works.
- [ ] In-guest: `nvidia-smi.exe` reports an RTX 4090 with full
      device memory.
- [ ] Bolt receiver moves into the guest cleanly; mouse + keyboard
      work in Windows; second host keyboard still drives the iGPU
      session.
- [ ] Shutdown Windows from inside guest → QEMU exits → handoff
      script restores driver and displays inside 10s.
- [ ] Repeat the enter/leave cycle 5 times without reboot.
- [ ] Sunshine still streams DP-3 capture after a leave (HDR
      patched KWin → genuine HDR pixels per
      [desktop.nix](../../system/desktop.nix)).
- [ ] After NixOS rebuild + reboot, `vm-curator` still finds the VM,
      `~/.config/vm-curator` is intact, the TB4 VM disk is intact.

---

## 10. Open follow-ups (out of scope for v1)

- **Looking Glass**: add IVSHMEM device to the QEMU args + LG client
  on the host. Useful for non-HDR productivity work in the VM. Adds
  `-device ivshmem-plain,memdev=ivshmem,bus=pcie.0` and a tmpfs
  shmem file managed by a systemd-tmpfiles entry.
- **Auto-rebind on resume from suspend**: if the host suspends with
  the GPU bound to vfio-pci (because we forgot to `leave`), the
  resume path needs to handle it. Add a sleep hook that calls
  `vfio leave` if the override is still set.
- **A second VM profile (Linux gaming, Bazzite)**: same flow,
  different launch.sh, useful for testing without touching the
  Windows install.
- **GPU LED off in VM mode**: the
  [gpu-led-off](FRACTAL-NORTH/gpu.nix#L52) systemd unit runs once
  at boot. If the VM re-enables the LED, add a leave-hook that
  re-runs `openrgb --noautoconnect -d 1 -m Off`.
- **Sunshine app entry**: add a "Windows VM" entry in Sunshine that
  invokes `nixos vfio enter && vm-curator launch win11-gaming`
  so the VM can be triggered from a Moonlight client. Useful for
  couch gaming.
- **CPU pinning + isolcpus**: if jitter shows up in frametime
  graphs, isolate 8 cores for the guest and pin emulator threads.
  Not before there's a measured problem.
