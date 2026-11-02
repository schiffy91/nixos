set -euo pipefail
device=$(findmnt -no SOURCE /mnt/recovery)
name=$(basename "$device")
partition=$(cat "/sys/class/block/$name/partition")
disk=/dev/$(lsblk -no PKNAME "$device")
case "$(uname -m)" in
  x86_64) fallback=BOOTX64.EFI ;;
  aarch64) fallback=BOOTAA64.EFI ;;
  *) exit 1 ;;
esac
loader='\EFI\BOOT\'"$fallback"
efibootmgr --create --disk "$disk" --part "$partition" \
  --label NixOSRecoveryTest --loader "$loader"
