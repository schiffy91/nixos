#!/usr/bin/env bash
set -euo pipefail
work_dir=$(realpath "$1")
nix_args=()
case ${2:-control} in
  control) fixture=tests/sleep-lock/gamescope-native ;;
  lock) fixture=tests/sleep-lock/gamescope-native-lock ;;
  resume-lock) fixture=tests/sleep-lock/gamescope-native-lock; nix_args=(--arg realSuspend true) ;;
  suspend-control) fixture=tests/sleep-lock/gamescope-native-suspend-control ;;
  *) echo 'Expected control, lock, resume-lock, or suspend-control fixture.' >&2; exit 1 ;;
esac
[[ $EUID != 0 && $(uname -m) == x86_64 ]]
unset SSH_AUTH_SOCK

# QEMU selects EGL's node explicitly, but its native-context renderer has no
# get_drm_fd callback: virglrenderer opens an amdgpu render node by driver name.
# Refuse ambiguous AMD hosts so both paths necessarily use the same GPU.
amd_nodes=()
for device in /sys/class/drm/renderD*; do
  [[ -r $device/device/vendor ]] || continue
  if [[ $(cat "$device/device/vendor") == 0x1002 && $(basename "$(readlink -f "$device/device/driver")") == amdgpu ]]; then
    amd_nodes+=("/dev/dri/$(basename "$device")")
  fi
done
[[ ${#amd_nodes[@]} == 1 ]] || { echo 'Expected exactly one AMD render node; refusing ambiguous GPU selection.' >&2; exit 1; }
render_node=${amd_nodes[0]}
[[ -r $render_node && -w $render_node && -r /dev/kvm && -w /dev/kvm ]]
[[ -r /run/opengl-driver/share/vulkan/icd.d/radeon_icd.x86_64.json ]]
printf 'Using existing AMD render-node access: %s (%s)\n' "$render_node" "$(readlink -f "/sys/class/drm/$(basename "$render_node")/device")"
# AMD native context is experimental. It does not acquire host DRM master,
# but a host driver hang/reset can affect other clients of this same iGPU.
printf 'Experimental AMD native-context control; bounded to two vCPUs, 4 GiB, and 600 seconds.\n'
driver=$(nix-build "$fixture" -A driver --argstr renderNode "$render_node" "${nix_args[@]}" --no-out-link --cores 4 --max-jobs 1)
scratch=$(mktemp -d /var/tmp/nixos-gamescope-native.XXXXXX)
trap 'rm -rf -- "$scratch"' EXIT
mkdir -p "$work_dir/output"
cd "$scratch"
export XDG_RUNTIME_DIR="$scratch" TMPDIR="$scratch"
unset DISPLAY WAYLAND_DISPLAY DBUS_SESSION_BUS_ADDRESS TESSDATA_PREFIX
# An absent DISPLAY makes the driver append -nographic, overriding EGL.
# The empty value avoids that heuristic without connecting to a desktop.
export DISPLAY=
export NIX_BUILD_CORES=4 OMP_THREAD_LIMIT=1
export VK_DRIVER_FILES=/run/opengl-driver/share/vulkan/icd.d/radeon_icd.x86_64.json
export LOGFILE="$work_dir/output/driver.xml"
# Build only the driver in Nix's sandbox; run it outside as the ordinary user.
# No sandbox exceptions or host device permission changes are needed.
timeout --kill-after=10s 600s "$driver/bin/nixos-test-driver" -o "$work_dir/output"
