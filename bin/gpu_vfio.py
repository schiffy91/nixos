#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import sys, time
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from lib import Shell, Utils

sh = Shell(root_required=True)

VM_NAME = "win11"
VM_XML = "/etc/nixos/vm/win11.xml"
KVMFR_DEV = "/dev/kvmfr0"
NVME_ID = "nvme-WD_BLACK_SN850X_4000GB_22461L800626"
NVME_PATH = f"/dev/disk/by-id/{NVME_ID}"
KBD_EVENT = "usb-Razer_Razer_BlackWidow_Lite-event-kbd"
MOUSE_EVENT = "usb-Logitech_USB_Receiver-if01-event-mouse"
GPU_PCI = "0000:01:00.0"
HOOK_DISPATCHER = "/etc/libvirt/hooks/qemu"
HOOK_START = f"/etc/libvirt/hooks/qemu.d/{VM_NAME}/prepare/begin/start.sh"
HOOK_REVERT = f"/etc/libvirt/hooks/qemu.d/{VM_NAME}/release/end/revert.sh"

##### State helpers #####

def vm_state():
    result = sh.run(f"virsh domstate {VM_NAME}", check=False)
    return Shell.stdout(result).strip() if result.returncode == 0 else "undefined"

def vm_defined():
    return sh.run(f"virsh dominfo {VM_NAME}", check=False).returncode == 0

def gpu_driver():
    p = Path(f"/sys/bus/pci/devices/{GPU_PCI}/driver")
    return p.resolve().name if p.is_symlink() else "none"

##### Preflight #####

def preflight():
    Utils.print("Preflight checks:")
    checks = [
        ("IOMMU enabled",      lambda: Path("/sys/kernel/iommu_groups/0").is_dir()),
        ("KVMFR device",       lambda: Path(KVMFR_DEV).is_char_device()),
        ("TB4 NVMe",           lambda: Path(NVME_PATH).exists()),
        ("Keyboard evdev",     lambda: Path(f"/dev/input/by-id/{KBD_EVENT}").exists()),
        ("Mouse evdev",        lambda: Path(f"/dev/input/by-id/{MOUSE_EVENT}").exists()),
        ("VM XML exists",      lambda: Path(VM_XML).exists()),
        ("VM defined",         vm_defined),
        ("Hook dispatcher",    lambda: Path(HOOK_DISPATCHER).exists()),
        ("start.sh hook",      lambda: Path(HOOK_START).exists()),
        ("revert.sh hook",     lambda: Path(HOOK_REVERT).exists()),
    ]
    all_ok = True
    for name, fn in checks:
        try: ok = fn()
        except Exception: ok = False
        Utils.print(f"  {'✓' if ok else '✗'} {name}")
        if not ok: all_ok = False
    return all_ok

##### Commands #####

def setup():
    Utils.print("=== VFIO VM Setup ===")
    if not Path(NVME_PATH).exists(): Utils.abort("Connect the TB4 NVMe before setup.")
    if vm_defined():
        Utils.log(f"Undefining old VM '{VM_NAME}'...")
        sh.run(f"virsh undefine {VM_NAME} --nvram", check=False)
    Utils.log(f"Defining VM '{VM_NAME}'...")
    sh.run(f"virsh define {VM_XML}")
    Utils.print(f"\nVM '{VM_NAME}' defined. Run: sudo gpu_vfio.py start")

def start():
    Utils.print("=== Starting VFIO VM ===")
    if vm_state() == "running":
        Utils.print("VM already running.")
        return
    if not vm_defined():
        Utils.log("VM not defined, running setup first...")
        setup()
    if not Path(NVME_PATH).exists(): Utils.abort("TB4 NVMe not connected.")
    Utils.log("Starting VM (libvirt hook detaches GPU)...")
    sh.run(f"virsh start {VM_NAME}")
    Utils.print(f"\nVM started. Connect: looking-glass-client -f {KVMFR_DEV}")

def stop():
    Utils.print("=== Stopping VFIO VM ===")
    state = vm_state()
    if state != "running":
        Utils.print(f"VM not running (state: {state})")
        return
    Utils.log("Sending ACPI shutdown...")
    sh.run(f"virsh shutdown {VM_NAME}", check=False)
    for _ in range(60):
        if vm_state() == "shut off": break
        time.sleep(2)
    else:
        Utils.log("Graceful timeout; forcing off...")
        sh.run(f"virsh destroy {VM_NAME}", check=False)
    Utils.print("VM stopped. Hook revert.sh restores host GPU + DM.")

def status():
    Utils.print("=== VFIO Status ===")
    Utils.print(f"  VM:       {vm_state()}")
    Utils.print(f"  GPU:      {gpu_driver()}")
    Utils.print(f"  KVMFR:    {'present' if Path(KVMFR_DEV).is_char_device() else 'missing'}")
    Utils.print(f"  NVMe:     {'connected' if Path(NVME_PATH).exists() else 'disconnected'}")

def check():
    Utils.print("All checks passed." if preflight() else "Some checks failed.")

def main():
    args = Utils.parse_args({
        "setup": [], "start": [], "stop": [], "status": [], "check": [],
    })
    {"setup": setup, "start": start, "stop": stop, "status": status, "check": check}[args.command]()

if __name__ == "__main__":
    main()
