#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import sys, os, json, time, re, shlex
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from lib import Shell, Utils

sh = Shell(root_required=False)

STATE_FILE = "/tmp/vfio_phase_state.json"
_cfg = json.loads(Path("/etc/nixos/config.json").read_text(encoding="utf-8"))
FLAKE_TARGET = f"{Path(_cfg['host_path']).stem}-{_cfg['target']}"
VM_NAME = "win11"
GPU_PCI = "0000:01:00.0"
AUDIO_PCI = "0000:01:00.1"
KVMFR_DEV = "/dev/kvmfr0"
NIXOS_DIR = "/etc/nixos"
ADMIN = "alexanderschiffhauer"

##### Check helpers #####

_stats = {"checks": 0, "failures": 0}

def check(name, predicate):
    _stats["checks"] += 1
    try: ok = bool(predicate())
    except Exception as e: ok = False; name = f"{name} [{type(e).__name__}: {e}]"
    Utils.print(f"  {'✓' if ok else '✗'} {name}")
    if not ok: _stats["failures"] += 1
    return ok

def heading(title):
    Utils.print(f"\n=== {title} ===")

def section(title):
    Utils.print(f"\n  --- {title} ---")

def save_state(phase, passed, detail=""):
    state = {}
    try: state = json.loads(Path(STATE_FILE).read_text(encoding="utf-8"))
    except Exception: pass
    state[f"phase_{phase}"] = {
        "passed": passed,
        "time": time.strftime("%Y-%m-%d %H:%M:%S"),
        "detail": detail,
    }
    Path(STATE_FILE).write_text(json.dumps(state, indent=2), encoding="utf-8")

def finish(phase):
    c, f = _stats["checks"], _stats["failures"]
    passed = f == 0
    save_state(phase, passed, f"{c - f}/{c} checks")
    Utils.print(f"\nPhase {phase}: {'PASS' if passed else 'FAIL'} — {c - f}/{c} checks passed")
    sys.exit(0 if passed else 1)

def run(cmd, sudo=False):
    return sh.run(cmd, check=False, sudo=sudo)

def stdout_of(cmd, sudo=False):
    return Shell.stdout(run(cmd, sudo=sudo))

##### Phase 1: build-only validation #####

def phase_1():
    heading(f"Phase 1: build-only validation (target: {FLAKE_TARGET})")
    Utils.log("Running nix build (this takes a few minutes)...")
    result = run(f"cd {NIXOS_DIR} && nix build .#nixosConfigurations.{FLAKE_TARGET}.config.system.build.toplevel --print-out-paths 2>&1", sudo=False)
    check("nix build exit 0", lambda: result.returncode == 0)
    out_path = Shell.stdout(result).strip().splitlines()[-1] if result.returncode == 0 else ""
    check("result symlink exists", lambda: Path(f"{NIXOS_DIR}/result").is_symlink())
    check("toplevel closure valid", lambda: out_path.startswith("/nix/store/"))
    if not out_path: finish(1)
    section("patched QEMU derivation")
    req = stdout_of(f"nix-store --query --requisites {out_path} 2>/dev/null")
    check("patched qemu-10 in closure", lambda: re.search(r"-qemu-10\.\d+\.\d+", req) is not None)
    section("libvirt QEMU hook")
    tmpfiles = stdout_of(f"cat {out_path}/etc/tmpfiles.d/00-nixos.conf 2>/dev/null")
    check("tmpfiles rule for hooks/qemu", lambda: "/var/lib/libvirt/hooks/qemu" in tmpfiles)
    m = re.search(r"(/nix/store/[^\s]+-qemu-hook)", tmpfiles)
    check("hook script path resolves", lambda: m is not None and Path(m.group(1)).exists())
    if m: check("hook script contains bind_vfio", lambda: "bind_vfio" in Path(m.group(1)).read_text(encoding="utf-8"))
    section("compiled ACPI tables")
    aml_out = stdout_of(f"nix-store --query --requisites {out_path} 2>/dev/null | xargs -I{{}} find {{}} -maxdepth 2 -name '*.aml' 2>/dev/null")
    check("fake_battery.aml in closure", lambda: "fake_battery.aml" in aml_out)
    check("spoofed_devices.aml in closure", lambda: "spoofed_devices.aml" in aml_out)
    section("kvmfr module")
    kvmfr_out = stdout_of(f"nix-store --query --requisites {out_path} 2>/dev/null | xargs -I{{}} find {{}} -name 'kvmfr*.ko*' 2>/dev/null | head -3")
    check("kvmfr kernel module in closure", lambda: "kvmfr" in kvmfr_out and ".ko" in kvmfr_out)
    section("etc symlinks for ACPI tables")
    etc_out = stdout_of(f"ls -la {out_path}/etc/acpi-spoofed-tables/ 2>/dev/null")
    check("etc/acpi-spoofed-tables/fake_battery.aml wired", lambda: "fake_battery.aml" in etc_out)
    check("etc/acpi-spoofed-tables/spoofed_devices.aml wired", lambda: "spoofed_devices.aml" in etc_out)
    finish(1)

##### Phase 2: switch + preflight #####

def phase_2():
    heading(f"Phase 2: switch + runtime preflight (target: {FLAKE_TARGET})")
    Utils.log("Running update.py (canonical rebuild path, answering 'n' to reboot prompt)...")
    r = run(f"echo 'n' | {NIXOS_DIR}/bin/update.py 2>&1", sudo=True)
    check("update.py exit 0", lambda: r.returncode == 0)
    if r.returncode != 0:
        Utils.print("  Last 40 lines of update output:")
        for line in Shell.stdout(r).splitlines()[-40:]: Utils.print(f"    {line}")
        finish(2)
    section("kernel / IOMMU")
    check("IOMMU groups exist", lambda: sh.is_dir("/sys/kernel/iommu_groups/0"))
    check("vfio-pci module available", lambda: run("modinfo vfio-pci").returncode == 0)
    check("kvmfr module loaded", lambda: "kvmfr" in stdout_of("lsmod"))
    check(f"{KVMFR_DEV} exists", lambda: sh.exists(KVMFR_DEV))
    section("libvirt")
    check("libvirtd active", lambda: stdout_of("systemctl is-active libvirtd").strip() == "active")
    check("/var/lib/libvirt/hooks/qemu symlink",
          lambda: sh.is_symlink("/var/lib/libvirt/hooks/qemu") and "nix/store" in sh.realpath("/var/lib/libvirt/hooks/qemu"))
    check("hook script executable",
          lambda: run("test -x /var/lib/libvirt/hooks/qemu").returncode == 0)
    section("ACPI spoofed tables")
    check("/etc/acpi-spoofed-tables/fake_battery.aml", lambda: sh.exists("/etc/acpi-spoofed-tables/fake_battery.aml"))
    check("/etc/acpi-spoofed-tables/spoofed_devices.aml", lambda: sh.exists("/etc/acpi-spoofed-tables/spoofed_devices.aml"))
    section("persistence")
    mounts = stdout_of("mount")
    check("/var/lib/libvirt persisted", lambda: "/var/lib/libvirt" in mounts)
    check("/etc/libvirt persisted", lambda: "/etc/libvirt" in mounts)
    section("user/group")
    groups = stdout_of(f"groups {ADMIN}")
    check("admin in 'kvm' group", lambda: re.search(r"\bkvm\b", groups) is not None)
    check("admin in 'input' group", lambda: re.search(r"\binput\b", groups) is not None)
    check("admin in 'libvirtd' group", lambda: re.search(r"\blibvirtd\b", groups) is not None)
    section("GPU still on host (pre-detach)")
    gpu_driver = Path(f"/sys/bus/pci/devices/{GPU_PCI}/driver")
    check("GPU driver = nvidia", lambda: gpu_driver.is_symlink() and gpu_driver.resolve().name == "nvidia")
    check("nvidia-smi works", lambda: run("nvidia-smi").returncode == 0)
    section("gpu_vfio.py check")
    r2 = run("/etc/nixos/bin/gpu_vfio.py check", sudo=True)
    check("gpu_vfio.py check passes", lambda: r2.returncode == 0 and "All checks passed" in Shell.stdout(r2))
    finish(2)

##### Phase 3: detach/reattach cycle (DESTRUCTIVE — kills display) #####

def phase_3():
    heading("Phase 3: GPU detach/reattach cycle (DESTRUCTIVE)")
    Utils.print("  WARNING: This kills the display manager.")
    Utils.print("  You must run this from SSH or a TTY.")
    if not sys.stdin.isatty(): Utils.print("  (non-interactive — proceeding)")
    else:
        answer = input("  Continue? [yes/NO]: ").strip().lower()
        if answer != "yes": Utils.abort("aborted by user")
    run("dmesg -C", sudo=True)
    section("cycle 1: detach")
    r = run("/etc/nixos/bin/gpu_vfio.py detach", sudo=True)
    check("detach exit 0", lambda: r.returncode == 0)
    check("GPU → vfio-pci", lambda: Path(f"/sys/bus/pci/devices/{GPU_PCI}/driver").resolve().name == "vfio-pci")
    check("Audio → vfio-pci", lambda: Path(f"/sys/bus/pci/devices/{AUDIO_PCI}/driver").resolve().name == "vfio-pci")
    lsmod = stdout_of("lsmod")
    check("no nvidia modules loaded", lambda: not re.search(r"^nvidia", lsmod, re.M))
    dm = stdout_of("dmesg", sudo=True)
    check("dmesg has no BUG:", lambda: "BUG:" not in dm)
    check("dmesg has no Hardware Error", lambda: "Hardware Error" not in dm)
    section("cycle 1: reattach")
    run("dmesg -C", sudo=True)
    r = run("/etc/nixos/bin/gpu_vfio.py attach", sudo=True)
    check("attach exit 0", lambda: r.returncode == 0)
    check("GPU → nvidia", lambda: Path(f"/sys/bus/pci/devices/{GPU_PCI}/driver").resolve().name == "nvidia")
    check("Audio → snd_hda_intel", lambda: Path(f"/sys/bus/pci/devices/{AUDIO_PCI}/driver").resolve().name == "snd_hda_intel")
    lsmod = stdout_of("lsmod")
    for mod in ["nvidia", "nvidia_drm", "nvidia_modeset", "nvidia_uvm"]:
        check(f"module {mod} loaded", lambda m=mod: re.search(rf"^{re.escape(m)}\b", lsmod, re.M) is not None)
    check("nvidia-smi works", lambda: run("nvidia-smi").returncode == 0)
    time.sleep(3)
    check("display-manager active", lambda: stdout_of("systemctl is-active display-manager").strip() == "active")
    dm = stdout_of("dmesg", sudo=True)
    check("reattach dmesg clean (no BUG:)", lambda: "BUG:" not in dm)
    section("cycle 2: detach (idempotency)")
    run("dmesg -C", sudo=True)
    r = run("/etc/nixos/bin/gpu_vfio.py detach", sudo=True)
    check("detach 2 exit 0", lambda: r.returncode == 0)
    check("GPU → vfio-pci (cycle 2)", lambda: Path(f"/sys/bus/pci/devices/{GPU_PCI}/driver").resolve().name == "vfio-pci")
    section("cycle 2: reattach (idempotency)")
    r = run("/etc/nixos/bin/gpu_vfio.py attach", sudo=True)
    check("attach 2 exit 0", lambda: r.returncode == 0)
    check("GPU → nvidia (cycle 2)", lambda: Path(f"/sys/bus/pci/devices/{GPU_PCI}/driver").resolve().name == "nvidia")
    check("nvidia-smi works (cycle 2)", lambda: run("nvidia-smi").returncode == 0)
    finish(3)

##### Phase state/report #####

def status():
    heading("VFIO phase state")
    try: state = json.loads(Path(STATE_FILE).read_text(encoding="utf-8"))
    except Exception: Utils.print("  (no state yet)"); return
    for k in sorted(state.keys()):
        v = state[k]
        mark = "✓" if v.get("passed") else "✗"
        Utils.print(f"  {mark} {k}: {v.get('detail','')} @ {v.get('time','')}")

def main():
    args = Utils.parse_args({
        "p1": [], "p2": [], "p3": [], "status": [],
    })
    cmd = args.command
    if os.environ.get("_VFIO_TEED") != "1" and cmd != "status":
        os.environ["_VFIO_TEED"] = "1"
        log_path = f"/tmp/vfio_{cmd}.log"
        quoted = " ".join(shlex.quote(a) for a in sys.argv)
        Path(log_path).write_text("", encoding="utf-8")
        Utils.print(f"(logging to {log_path})")
        os.execvp("sh", ["sh", "-c", f"exec {quoted} 2>&1 | tee {shlex.quote(log_path)}"])
    if cmd == "p1": phase_1()
    elif cmd == "p2": phase_2()
    elif cmd == "p3": phase_3()
    elif cmd == "status": status()

if __name__ == "__main__":
    main()
