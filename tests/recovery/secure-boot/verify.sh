set -euo pipefail
for unit in generate-sb-keys.service nixos-recovery-boot-entry.service; do
  systemctl start "$unit"
  test "$(systemctl show -p Result --value "$unit")" = success
done
case "$(uname -m)" in
  x86_64) fallback=BOOTX64.EFI ;;
  aarch64) fallback=BOOTAA64.EFI ;;
  *) exit 1 ;;
esac
certificate=/var/lib/sbctl/keys/db/db.pem
for root in /boot /recovery; do
  grep -Fx 'efi /EFI/recovery/nixos-recovery.efi' "$root/loader/entries/nixos-recovery.conf"
  if grep -E '^(linux|initrd|options) ' "$root/loader/entries/nixos-recovery.conf"; then exit 1; fi
  sbverify --cert "$certificate" "$root/EFI/recovery/nixos-recovery.efi"
  if test -e "$root/EFI/recovery/kernel.efi" || test -e "$root/EFI/recovery/initrd"; then exit 1; fi
done
sbverify --cert "$certificate" "/recovery/EFI/BOOT/$fallback"
sha256sum --check /recovery/.nixos-recovery-checksums
# A full destination must fail before publishing anything to the main ESP.
before=$(sha256sum /boot/EFI/recovery/nixos-recovery.efi /boot/loader/entries/nixos-recovery.conf)
mount -t tmpfs -o size=1M tmpfs /recovery/EFI/recovery
if systemctl restart nixos-recovery-boot-entry; then
  umount /recovery/EFI/recovery
  exit 1
fi
umount /recovery/EFI/recovery
test "$before" = "$(sha256sum /boot/EFI/recovery/nixos-recovery.efi /boot/loader/entries/nixos-recovery.conf)"
systemctl reset-failed nixos-recovery-boot-entry
systemctl start nixos-recovery-boot-entry
# A matching declaration is insufficient: damage an installed binary and
# require the real service to detect and repair it.
printf broken > "/recovery/EFI/BOOT/$fallback"
systemctl restart nixos-recovery-boot-entry
sbverify --cert "$certificate" "/recovery/EFI/BOOT/$fallback"
sha256sum --check /recovery/.nixos-recovery-checksums
# Changing the embedded initrd must invalidate the signed image. The PE
# section offset is read from objdump instead of assuming a file layout.
offset=$(objdump -h /recovery/EFI/recovery/nixos-recovery.efi | awk '$2 == ".initrd" { print $6 }')
test -n "$offset"
printf tampered | dd of=/recovery/EFI/recovery/nixos-recovery.efi bs=1 seek="$((16#$offset))" conv=notrunc status=none
if sbverify --cert "$certificate" /recovery/EFI/recovery/nixos-recovery.efi; then exit 1; fi
systemctl restart nixos-recovery-boot-entry
sbverify --cert "$certificate" /recovery/EFI/recovery/nixos-recovery.efi
sha256sum --check /recovery/.nixos-recovery-checksums
bootctl list | grep -F nixos-recovery.conf
echo recovery-secure-entry-ok
