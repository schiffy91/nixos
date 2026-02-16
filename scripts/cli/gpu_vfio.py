#! /usr/bin/env nix-shell
#! nix-shell -i python3 -p python3
import os
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from core import Utils, Shell

sh = Shell(root_required=True)

GPU_PCI_ID = "0000:01:00.0"
GPU_AUDIO_PCI_ID = "0000:01:00.1"
GPU_VENDOR_DEVICE = "10de:2684"
AUDIO_VENDOR_DEVICE = "10de:22ba"


def get_driver(pci_id):
    driver_path = f"/sys/bus/pci/devices/{pci_id}/driver"
    if sh.is_symlink(driver_path):
        return sh.basename(sh.realpath(driver_path))
    return "none"


def check_iommu():
    result = sh.run("dmesg | grep -q 'IOMMU enabled'", check=False)
    if result.returncode != 0:
        Utils.abort("IOMMU is not enabled. Add 'amd_iommu=on iommu=pt' "
                    "to kernel parameters.")
    Utils.log("IOMMU is enabled")


def unbind_device(pci_id):
    driver = get_driver(pci_id)
    if driver != "none":
        Utils.log(f"Unbinding {pci_id} from {driver}...")
        path = f"/sys/bus/pci/devices/{pci_id}/driver/unbind"
        sh.file_write(path, pci_id)


def bind_device(pci_id, driver, vendor_device=None):
    Utils.log(f"Binding {pci_id} to {driver}...")
    if vendor_device:
        new_id = f"/sys/bus/pci/drivers/{driver}/new_id"
        sh.run(f"echo '{vendor_device}' > {new_id}", check=False)
    sh.run(f"echo '{pci_id}' > /sys/bus/pci/drivers/{driver}/bind")


def detach():
    Utils.log("Detaching NVIDIA GPU for VFIO passthrough...")
    check_iommu()
    if get_driver(GPU_PCI_ID) == "vfio-pci":
        Utils.log("GPU already bound to vfio-pci")
        return
    unbind_device(GPU_PCI_ID)
    unbind_device(GPU_AUDIO_PCI_ID)
    time.sleep(1)
    bind_device(GPU_PCI_ID, "vfio-pci", GPU_VENDOR_DEVICE)
    bind_device(GPU_AUDIO_PCI_ID, "vfio-pci", AUDIO_VENDOR_DEVICE)
    Utils.log(f"GPU: {get_driver(GPU_PCI_ID)}")
    Utils.log(f"Audio: {get_driver(GPU_AUDIO_PCI_ID)}")


def attach():
    Utils.log("Reattaching NVIDIA GPU to host...")
    if get_driver(GPU_PCI_ID) == "nvidia":
        Utils.log("GPU already bound to nvidia driver")
        return
    unbind_device(GPU_PCI_ID)
    unbind_device(GPU_AUDIO_PCI_ID)
    time.sleep(1)
    bind_device(GPU_PCI_ID, "nvidia")
    bind_device(GPU_AUDIO_PCI_ID, "snd_hda_intel")
    Utils.log(f"GPU: {get_driver(GPU_PCI_ID)}")
    Utils.log(f"Audio: {get_driver(GPU_AUDIO_PCI_ID)}")
    Utils.log("You may need to restart display-manager")


def status():
    gpu = get_driver(GPU_PCI_ID)
    audio = get_driver(GPU_AUDIO_PCI_ID)
    Utils.print(f"GPU ({GPU_PCI_ID}): {gpu}")
    Utils.print(f"Audio ({GPU_AUDIO_PCI_ID}): {audio}")
    if gpu == "vfio-pci":
        Utils.print("Status: Detached (ready for VM passthrough)")
    elif gpu == "nvidia":
        Utils.print("Status: Attached to host (active for rendering)")
    else:
        Utils.print("Status: Unknown state")


def main():
    if len(sys.argv) < 2:
        Utils.abort("Usage: gpu_vfio.py [attach|detach|status]")
    match sys.argv[1]:
        case "detach":
            detach()
        case "attach":
            attach()
        case "status":
            status()
        case _:
            Utils.abort("Usage: gpu_vfio.py [attach|detach|status]")


if __name__ == "__main__":
    main()
