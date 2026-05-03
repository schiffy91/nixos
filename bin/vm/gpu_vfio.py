#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import sys, time
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent))
from lib import Config, Shell, Utils

sh = Shell(root_required=True)

VM_NAME = Config.eval("config.settings.vfio.vmName")
GPU_PCI = Config.eval("config.settings.vfio.gpuPci")
NVME_ID = Config.eval("config.settings.vfio.nvmeId")
KBD_EVENT = Config.eval("config.settings.vfio.keyboardEvent")
MOUSE_EVENT = Config.eval("config.settings.vfio.mouseEvent")
VM_XML = f"/etc/nixos/bin/vm/{VM_NAME}.xml"
NVME_PATH = f"/dev/disk/by-id/{NVME_ID}"
KVMFR_DEV = "/dev/kvmfr0"

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
        ("GPU vfio-bound",     lambda: gpu_driver() == "vfio-pci"),
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
    if gpu_driver() != "vfio-pci":
        Utils.abort("GPU not bound to vfio-pci. Reboot into the 'vfio' specialisation.")
    Utils.log("Starting VM...")
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
    Utils.print("VM stopped.")

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
