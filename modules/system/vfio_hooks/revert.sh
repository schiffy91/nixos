#!/usr/bin/env bash
# Single-GPU passthrough: restore host after VM stop
# Libvirt has already detached devices from vfio-pci by the time this runs
set -x
exec >> /var/log/libvirt/vfio-revert.log 2>&1
echo "=== $(date -Is) revert.sh begin ==="
modprobe nvidia
modprobe nvidia_modeset
modprobe nvidia_uvm
modprobe nvidia_drm
echo 1 > /sys/class/vtconsole/vtcon0/bind 2>/dev/null || true
echo 1 > /sys/class/vtconsole/vtcon1/bind 2>/dev/null || true
echo efi-framebuffer.0 > /sys/bus/platform/drivers/efi-framebuffer/bind 2>/dev/null || true
nvidia-smi -L > /dev/null 2>&1 || true
systemctl start display-manager.service
echo "=== $(date -Is) revert.sh end ==="
