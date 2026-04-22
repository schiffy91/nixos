#!/usr/bin/env bash
# Single-GPU passthrough: prepare host before VM start
# Pattern: joeknock90/QaidVoid/PassthroughPOST community standard
# Runs as root from libvirtd context (system.slice)
set -x
exec >> /var/log/libvirt/vfio-start.log 2>&1
echo "=== $(date -Is) start.sh begin ==="
systemctl stop display-manager.service
echo 0 > /sys/class/vtconsole/vtcon0/bind 2>/dev/null || true
echo 0 > /sys/class/vtconsole/vtcon1/bind 2>/dev/null || true
echo efi-framebuffer.0 > /sys/bus/platform/drivers/efi-framebuffer/unbind 2>/dev/null || true
sleep 2
modprobe -r nvidia_drm   2>/dev/null || true
modprobe -r nvidia_uvm   2>/dev/null || true
modprobe -r nvidia_modeset 2>/dev/null || true
modprobe -r nvidia       2>/dev/null || true
modprobe vfio-pci
echo "=== $(date -Is) start.sh end ==="
