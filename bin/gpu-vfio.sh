#!/usr/bin/env bash
# Dynamic NVIDIA RTX 4090 VFIO Passthrough Script
# Usage: gpu-vfio.sh [attach|detach|status]

set -euo pipefail

# GPU PCI Address (from lspci: 01:00.0)
GPU_PCI_ID="0000:01:00.0"
GPU_AUDIO_PCI_ID="0000:01:00.1"  # NVIDIA HDMI Audio

# PCI IDs for RTX 4090 (from lspci -nn)
GPU_VENDOR_DEVICE="10de:2684"
AUDIO_VENDOR_DEVICE="10de:22ba"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() { echo -e "${GREEN}[INFO]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

check_iommu() {
    if ! dmesg | grep -q "IOMMU enabled"; then
        log_error "IOMMU is not enabled! Add 'amd_iommu=on iommu=pt' to kernel parameters"
        exit 1
    fi
    log_info "IOMMU is enabled ✓"
}

get_driver() {
    local pci_id=$1
    if [ -e "/sys/bus/pci/devices/$pci_id/driver" ]; then
        basename "$(readlink "/sys/bus/pci/devices/$pci_id/driver")"
    else
        echo "none"
    fi
}

unbind_device() {
    local pci_id=$1
    local driver=$(get_driver "$pci_id")

    if [ "$driver" != "none" ]; then
        log_info "Unbinding $pci_id from $driver..."
        echo "$pci_id" > "/sys/bus/pci/devices/$pci_id/driver/unbind"
    fi
}

bind_to_vfio() {
    local pci_id=$1
    local vendor_device=$2

    log_info "Binding $pci_id to vfio-pci..."
    echo "$vendor_device" > /sys/bus/pci/drivers/vfio-pci/new_id || true
    echo "$pci_id" > /sys/bus/pci/drivers/vfio-pci/bind
}

bind_to_nvidia() {
    local pci_id=$1

    log_info "Binding $pci_id to nvidia..."
    echo "$pci_id" > /sys/bus/pci/drivers/nvidia/bind
}

bind_to_snd_hda() {
    local pci_id=$1

    log_info "Binding $pci_id to snd_hda_intel..."
    echo "$pci_id" > /sys/bus/pci/drivers/snd_hda_intel/bind
}

detach_gpu() {
    log_info "Detaching NVIDIA RTX 4090 for VFIO passthrough..."

    check_iommu

    # Check current driver
    gpu_driver=$(get_driver "$GPU_PCI_ID")
    audio_driver=$(get_driver "$GPU_AUDIO_PCI_ID")

    log_info "Current GPU driver: $gpu_driver"
    log_info "Current Audio driver: $audio_driver"

    if [ "$gpu_driver" = "vfio-pci" ]; then
        log_warn "GPU already bound to vfio-pci"
        return 0
    fi

    # Unbind from current drivers
    unbind_device "$GPU_PCI_ID"
    unbind_device "$GPU_AUDIO_PCI_ID"

    # Wait for unbind
    sleep 1

    # Bind to VFIO
    bind_to_vfio "$GPU_PCI_ID" "$GPU_VENDOR_DEVICE"
    bind_to_vfio "$GPU_AUDIO_PCI_ID" "$AUDIO_VENDOR_DEVICE"

    log_info "✓ GPU detached successfully! Ready for VM passthrough"
    log_info "  GPU: $(get_driver "$GPU_PCI_ID")"
    log_info "  Audio: $(get_driver "$GPU_AUDIO_PCI_ID")"
}

attach_gpu() {
    log_info "Reattaching NVIDIA RTX 4090 to host..."

    # Check current driver
    gpu_driver=$(get_driver "$GPU_PCI_ID")

    if [ "$gpu_driver" = "nvidia" ]; then
        log_warn "GPU already bound to nvidia driver"
        return 0
    fi

    # Unbind from VFIO
    unbind_device "$GPU_PCI_ID"
    unbind_device "$GPU_AUDIO_PCI_ID"

    # Wait for unbind
    sleep 1

    # Bind back to nvidia/snd_hda_intel
    bind_to_nvidia "$GPU_PCI_ID"
    bind_to_snd_hda "$GPU_AUDIO_PCI_ID"

    log_info "✓ GPU reattached successfully!"
    log_info "  GPU: $(get_driver "$GPU_PCI_ID")"
    log_info "  Audio: $(get_driver "$GPU_AUDIO_PCI_ID")"

    log_warn "You may need to restart the display manager: sudo systemctl restart display-manager"
}

show_status() {
    gpu_driver=$(get_driver "$GPU_PCI_ID")
    audio_driver=$(get_driver "$GPU_AUDIO_PCI_ID")

    echo "=== NVIDIA RTX 4090 Status ==="
    echo "GPU ($GPU_PCI_ID): $gpu_driver"
    echo "Audio ($GPU_AUDIO_PCI_ID): $audio_driver"
    echo ""

    if [ "$gpu_driver" = "vfio-pci" ]; then
        echo "Status: 🔴 Detached (Ready for VM passthrough)"
    elif [ "$gpu_driver" = "nvidia" ]; then
        echo "Status: 🟢 Attached to host (Active for rendering)"
    else
        echo "Status: ⚠️  Unknown state"
    fi
}

case "${1:-}" in
    detach)
        if [ "$EUID" -ne 0 ]; then
            log_error "Please run as root: sudo $0 detach"
            exit 1
        fi
        detach_gpu
        ;;
    attach)
        if [ "$EUID" -ne 0 ]; then
            log_error "Please run as root: sudo $0 attach"
            exit 1
        fi
        attach_gpu
        ;;
    status)
        show_status
        ;;
    *)
        echo "Usage: $0 [attach|detach|status]"
        echo ""
        echo "Commands:"
        echo "  detach  - Unbind GPU from nvidia driver and bind to vfio-pci (for VM use)"
        echo "  attach  - Unbind GPU from vfio-pci and bind back to nvidia (return to host)"
        echo "  status  - Show current GPU driver binding"
        echo ""
        echo "Example workflow:"
        echo "  1. sudo $0 detach          # Prepare GPU for VM"
        echo "  2. Start your VM with GPU passthrough"
        echo "  3. Stop the VM"
        echo "  4. sudo $0 attach          # Return GPU to host"
        exit 1
        ;;
esac
