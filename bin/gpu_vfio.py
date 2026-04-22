#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import sys, time
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from lib import Shell, Utils

sh = Shell(root_required=True)

GPU_PCI = "0000:01:00.0"
AUDIO_PCI = "0000:01:00.1"
GPU_VID = "10de:2684"
AUDIO_VID = "10de:22ba"
VM_NAME = "win11"
VM_XML = "/etc/nixos/vm/win11.xml"
KVMFR_DEV = "/dev/kvmfr0"
NVME_ID = "nvme-WD_BLACK_SN850X_4000GB_22461L800626"
NVME_PATH = f"/dev/disk/by-id/{NVME_ID}"
KBD_EVENT = "usb-Razer_Razer_BlackWidow_Lite-event-kbd"
MOUSE_EVENT = "usb-Logitech_USB_Receiver-if01-event-mouse"
NVIDIA_MODULES = ["nvidia_drm", "nvidia_modeset", "nvidia_uvm", "nvidia"]

##### Driver Management #####

def get_driver(pci_id):
    driver_path = f"/sys/bus/pci/devices/{pci_id}/driver"
    if sh.is_symlink(driver_path):
        return sh.basename(sh.realpath(driver_path))
    return "none"

def unbind_device(pci_id):
    driver = get_driver(pci_id)
    if driver != "none":
        Utils.log(f"Unbinding {pci_id} from {driver}...")
        sh.run(f"echo '{pci_id}' > /sys/bus/pci/devices/{pci_id}/driver/unbind")

def bind_device(pci_id, driver):
    Utils.log(f"Binding {pci_id} to {driver}...")
    sh.run(f"echo '{driver}' > /sys/bus/pci/devices/{pci_id}/driver_override")
    sh.run(f"echo '{pci_id}' > /sys/bus/pci/drivers_probe")
    sh.run(f"echo '' > /sys/bus/pci/devices/{pci_id}/driver_override")

##### GPU Attach/Detach #####

def check_iommu():
    if not sh.is_dir("/sys/kernel/iommu_groups/0"):
        Utils.abort("IOMMU is not enabled. Add 'amd_iommu=on iommu=pt' to kernel parameters.")

def stop_display_manager():
    Utils.log("Stopping display manager...")
    sh.run("systemctl stop display-manager", check=False)

def start_display_manager():
    Utils.log("Starting display manager...")
    sh.run("systemctl start display-manager", check=False)

def unbind_vtconsoles():
    Utils.log("Unbinding VT consoles...")
    result = sh.run("find /sys/devices/virtual/vtconsole -name 'bind' 2>/dev/null", check=False)
    for path in Shell.stdout(result).splitlines():
        if not path.strip(): continue
        sh.run(f"echo 0 > {path.strip()}", check=False)

def unbind_efi_framebuffer():
    Utils.log("Unbinding EFI framebuffer...")
    sh.run("echo efi-framebuffer.0 > /sys/bus/platform/drivers/efi-framebuffer/unbind", check=False)

def kill_gpu_processes():
    NVIDIA_DEVS = "/dev/nvidia0 /dev/nvidiactl /dev/nvidia-modeset /dev/nvidia-uvm /dev/nvidia-uvm-tools"
    DRI_DEVS = "/dev/dri/card0 /dev/dri/renderD128 /dev/dri/card1 /dev/dri/renderD129"
    devs = f"{NVIDIA_DEVS} {DRI_DEVS}"
    Utils.log("Killing GPU processes (graceful)...")
    sh.run(f"fuser -k {devs} 2>/dev/null", check=False)
    time.sleep(5)
    Utils.log("Killing GPU processes (force)...")
    sh.run(f"fuser -k -9 {devs} 2>/dev/null", check=False)
    time.sleep(2)

def unload_nvidia():
    Utils.log("Tearing down NVIDIA for VFIO passthrough...")
    kill_gpu_processes()
    unbind_vtconsoles()
    unbind_efi_framebuffer()
    Utils.log("Unloading NVIDIA kernel modules...")
    for mod in NVIDIA_MODULES:
        sh.run(f"modprobe -r {mod}", check=False)
    still_loaded = [m for m in NVIDIA_MODULES if sh.run(f"lsmod | grep -q '^{m} '", check=False).returncode == 0]
    if not still_loaded: return
    Utils.log(f"Still loaded: {', '.join(still_loaded)}. Retrying...")
    kill_gpu_processes()
    for mod in NVIDIA_MODULES:
        sh.run(f"modprobe -r {mod}", check=False)
    still_loaded = [m for m in NVIDIA_MODULES if sh.run(f"lsmod | grep -q '^{m} '", check=False).returncode == 0]
    if still_loaded:
        Utils.abort(f"Failed to unload {', '.join(still_loaded)}. Try: lsof /dev/nvidia0 /dev/dri/card0")

def detach():
    Utils.log("Detaching NVIDIA GPU for VFIO passthrough...")
    check_iommu()
    if get_driver(GPU_PCI) == "vfio-pci":
        Utils.log("GPU already bound to vfio-pci")
        return
    stop_display_manager()
    unload_nvidia()
    unbind_device(GPU_PCI)
    unbind_device(AUDIO_PCI)
    time.sleep(1)
    bind_device(GPU_PCI, "vfio-pci")
    bind_device(AUDIO_PCI, "vfio-pci")
    Utils.log(f"GPU: {get_driver(GPU_PCI)}, Audio: {get_driver(AUDIO_PCI)}")

def attach():
    Utils.log("Reattaching NVIDIA GPU to host...")
    if get_driver(GPU_PCI) == "nvidia":
        Utils.log("GPU already bound to nvidia driver")
        return
    unbind_device(GPU_PCI)
    unbind_device(AUDIO_PCI)
    time.sleep(1)
    bind_device(GPU_PCI, "nvidia")
    bind_device(AUDIO_PCI, "snd_hda_intel")
    Utils.log(f"GPU: {get_driver(GPU_PCI)}, Audio: {get_driver(AUDIO_PCI)}")
    start_display_manager()

##### VM State #####

def vm_state():
    result = sh.run(f"virsh domstate {VM_NAME}", check=False)
    return Shell.stdout(result).strip() if result.returncode == 0 else "undefined"

def vm_defined():
    return sh.run(f"virsh dominfo {VM_NAME}", check=False).returncode == 0

##### Preflight Checks #####

def preflight():
    Utils.print("Preflight checks:")
    checks = [
        ("IOMMU",          lambda: sh.is_dir("/sys/kernel/iommu_groups/0")),
        ("KVMFR device",   lambda: sh.exists(KVMFR_DEV)),
        ("TB4 NVMe",       lambda: sh.exists(NVME_PATH)),
        ("Keyboard evdev", lambda: sh.exists(f"/dev/input/by-id/{KBD_EVENT}")),
        ("Mouse evdev",    lambda: sh.exists(f"/dev/input/by-id/{MOUSE_EVENT}")),
        ("VM XML exists",  lambda: sh.exists(VM_XML)),
        ("VM defined",     vm_defined),
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
    if not sh.exists(KVMFR_DEV):
        Utils.abort("KVMFR not available. Run nixos-rebuild switch first.")
    if not sh.exists(NVME_PATH):
        Utils.abort("Connect the TB4 NVMe before setup.")
    if vm_defined():
        Utils.log(f"Undefining old VM '{VM_NAME}'...")
        sh.run(f"virsh undefine {VM_NAME} --nvram", check=False)
    Utils.log(f"Defining VM '{VM_NAME}'...")
    sh.run(f"virsh define {VM_XML}")
    Utils.print(f"\nVM '{VM_NAME}' is defined. Next steps:")
    Utils.print("  1. sudo gpu_vfio.py start")
    Utils.print("  2. sudo gpu_vfio.py guide   (for Windows setup instructions)")

def start():
    Utils.print("=== Starting VFIO VM ===")
    state = vm_state()
    if state == "running":
        Utils.print("VM is already running")
        Utils.print(f"  Connect: looking-glass-client -f {KVMFR_DEV}")
        return
    if not vm_defined():
        Utils.log("VM not defined, running setup first...")
        setup()
    errors = []
    if not sh.exists(NVME_PATH): errors.append("TB4 NVMe not connected")
    if not sh.exists(KVMFR_DEV): errors.append("KVMFR device missing")
    for name, dev in [("Keyboard", KBD_EVENT), ("Mouse", MOUSE_EVENT)]:
        if not sh.exists(f"/dev/input/by-id/{dev}"): errors.append(f"{name} not found")
    if errors:
        for e in errors: Utils.log_error(e)
        Utils.abort("Fix the above issues before starting.")
    detach()
    Utils.log("Starting VM...")
    sh.run(f"virsh start {VM_NAME}")
    Utils.print("\n=== VM Started ===")
    Utils.print(f"  Display:  looking-glass-client -f {KVMFR_DEV}")
    Utils.print("  Input:    Left Ctrl + Right Ctrl to toggle host/guest")
    Utils.print("  Stop:     sudo gpu_vfio.py stop")

def stop():
    Utils.print("=== Stopping VFIO VM ===")
    state = vm_state()
    if state != "running":
        Utils.log(f"VM is not running (state: {state})")
        attach()
        return
    Utils.log("Sending ACPI shutdown to VM...")
    sh.run(f"virsh shutdown {VM_NAME}", check=False)
    for i in range(30):
        if vm_state() == "shut off": break
        Utils.print_inline(f"  Waiting for shutdown... {30-i}s")
        time.sleep(2)
    else:
        Utils.print("")
        Utils.log("Graceful shutdown timed out, forcing off...")
        sh.run(f"virsh destroy {VM_NAME}", check=False)
        time.sleep(1)
    Utils.print("")
    attach()
    Utils.print("VM stopped, GPU reattached to host.")

def gpu_status():
    driver = get_driver(GPU_PCI)
    if driver == "vfio-pci": return "Detached (vfio-pci)"
    if driver == "nvidia": return "Attached (nvidia)"
    return f"Unknown ({driver})"

def status():
    Utils.print("=== VFIO Status ===")
    Utils.print(f"  VM:       {vm_state()}")
    Utils.print(f"  GPU:      {gpu_status()}")
    Utils.print(f"  Audio:    {get_driver(AUDIO_PCI)}")
    Utils.print(f"  KVMFR:    {'present' if sh.exists(KVMFR_DEV) else 'missing'}")
    Utils.print(f"  NVMe:     {'connected' if sh.exists(NVME_PATH) else 'disconnected'}")
    Utils.print(f"  Keyboard: {'connected' if sh.exists(f'/dev/input/by-id/{KBD_EVENT}') else 'missing'}")
    Utils.print(f"  Mouse:    {'connected' if sh.exists(f'/dev/input/by-id/{MOUSE_EVENT}') else 'missing'}")

def nvme():
    Utils.print("External NVMe devices (excluding nvme0):")
    result = sh.run("ls -1 /dev/disk/by-id/ | grep nvme | grep -v nvme0 | grep -v part", check=False)
    output = Shell.stdout(result).strip()
    if output:
        for line in output.splitlines():
            active = " (configured)" if NVME_ID in line else ""
            Utils.print(f"  /dev/disk/by-id/{line}{active}")
    else:
        Utils.print("  No external NVMe devices found")

def check():
    all_ok = preflight()
    Utils.print("")
    Utils.print("All checks passed. Ready to start." if all_ok else "Some checks failed. Fix issues above before starting.")

def guide():
    Utils.print("""
=== VFIO Windows VM Setup Guide ===

PREREQUISITES:
  - Run 'sudo nixos-rebuild switch' to apply NixOS changes
  - Connect TB4 NVMe with Windows installed
  - Have your BitLocker recovery key ready

FIRST-TIME SETUP:
  sudo gpu_vfio.py check    # verify all hardware is detected
  sudo gpu_vfio.py setup    # define the VM in libvirt
  sudo gpu_vfio.py start    # detach GPU + start VM

INITIAL BOOT (do these INSIDE Windows):

  1. BitLocker Recovery
     Windows will boot to BitLocker recovery screen.
     Enter your 48-digit recovery key.
     (Find it at: https://account.microsoft.com/devices/recoverykey)

  2. Re-enroll BitLocker with virtual TPM (Admin PowerShell):
     manage-bde -protectors -delete C: -type TPMProtector
     manage-bde -protectors -add C: -tpm

  3. Install NVIDIA Drivers
     The VM sees the real RTX 4090. Download drivers from nvidia.com.

  4. Install Looking Glass Host (B7)
     Download from: https://looking-glass.io/downloads
     Install and run as administrator. It will register as a Windows service.
     Verify: looking-glass-host.exe should show "Connected to KVMFR"

  5. Install VirtIO Drivers (optional, for future virtio devices)
     Mount the virtio-win ISO in virt-manager if needed.

  6. Disable Hyper-V (Admin CMD):
     bcdedit /set hypervisorlaunchtype off

  7. Anti-Cheat Registry Cleanup (Admin CMD):
     reg delete "HKLM\\HARDWARE\\ACPI\\DSDT\\BOCHS_" /f 2>nul
     reg delete "HKLM\\HARDWARE\\ACPI\\FADT\\BOCHS_" /f 2>nul
     reg delete "HKLM\\HARDWARE\\ACPI\\RSDT\\BOCHS_" /f 2>nul
     reg add "HKLM\\SOFTWARE\\Policies\\Microsoft\\Windows\\DeviceGuard" /v EnableVirtualizationBasedSecurity /t REG_DWORD /d 0 /f

  8. Verify Anti-Cheat Spoofing (inside Windows):
     - msinfo32: should show ASUS as manufacturer, not QEMU
     - Device Manager: no "QEMU" or "VirtIO" devices
     - Download Pafish: https://github.com/a0rtega/pafish/releases
       Run it — all VM detection checks should PASS

  9. Reboot Windows, then remove SPICE graphics from XML:
     (Only needed if you uncommented SPICE for initial setup)
     sudo virsh edit win11
     Delete the <graphics> and <video type='qxl'> sections.

DAILY USAGE:
  sudo gpu_vfio.py start     # detach GPU, boot VM
  looking-glass-client -f /dev/kvmfr0   # connect display
  [Left Ctrl + Right Ctrl]   # toggle keyboard/mouse host<->guest
  sudo gpu_vfio.py stop      # shutdown VM, reattach GPU

TROUBLESHOOTING:
  sudo gpu_vfio.py check     # run all preflight checks
  sudo gpu_vfio.py status    # show current state
  sudo gpu_vfio.py attach    # force reattach GPU to host
  sudo gpu_vfio.py detach    # force detach GPU for VM
  virsh console win11        # serial console to VM
  journalctl -u libvirtd     # libvirt logs
""")

def main():
    args = Utils.parse_args({
        "setup": [], "start": [], "stop": [], "status": [],
        "attach": [], "detach": [], "check": [], "nvme": [], "guide": []
    })
    commands = {
        "setup": setup, "start": start, "stop": stop, "status": status,
        "attach": attach, "detach": detach, "check": check, "nvme": nvme, "guide": guide
    }
    commands[args.command]()

if __name__ == "__main__":
    main()
