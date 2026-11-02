set -euo pipefail
cd /etc/nixos
bootctl status | grep -F 'Secure Boot: enabled'
# Root can evaluate the admin-owned flake after its mutable home is reset.
git config --system --get-all safe.directory | grep -Fx /etc/nixos
GIT_CONFIG_GLOBAL=/dev/null git status --porcelain >/dev/null
# Standard-Boot leaves type-1 entries behind. Even a newer version must not
# displace the signed type-2 default selected by the declarative loader config.
legacy=/boot/loader/entries/nixos-generation-999999.conf
mkdir -p /boot/loader/entries
trap 'rm -f "$legacy"' EXIT
case "$(uname -m)" in
  x86_64) fallback=BOOTX64.EFI ;;
  aarch64) fallback=BOOTAA64.EFI ;;
  *) exit 1 ;;
esac
cat > "$legacy" <<EOF
title Stale Standard-Boot test entry
sort-key nixos
version 999999
linux /EFI/BOOT/$fallback
EOF
nixosctl secure-boot status
rm "$legacy"
trap - EXIT
boot_files() { find /boot -type f -print0 | sort -z | xargs -0 sha256sum; }
before=$(boot_files)
cp config.json /var/tmp/config-before-downgrade.json
if nixosctl secure-boot disable > /var/tmp/disable.log 2>&1; then
  cat /var/tmp/disable.log; exit 1
fi
grep -F 'Disable Secure Boot in firmware' /var/tmp/disable.log
cmp config.json /var/tmp/config-before-downgrade.json
standard=$(nix build --no-link --print-out-paths .#nixosConfigurations.VM-TEST-Standard-Boot.config.system.build.toplevel)
for action in boot switch; do
  if "$standard/bin/switch-to-configuration" "$action" > /var/tmp/downgrade.log 2>&1; then
    cat /var/tmp/downgrade.log; exit 1
  fi
  grep -F 'Refusing Standard-Boot' /var/tmp/downgrade.log
done
# Even callers bypassing pre-switch checks must not write unsigned files.
if NIXOS_NO_CHECK=1 "$standard/bin/switch-to-configuration" boot > /var/tmp/downgrade.log 2>&1; then
  cat /var/tmp/downgrade.log; exit 1
fi
grep -F 'Refusing Standard-Boot' /var/tmp/downgrade.log
# Missing/unreadable efivarfs is not firmware without Secure Boot support.
# Hide it only in a private mount namespace; never change firmware variables.
unshare --mount --propagation private bash -s -- "$standard" <<'EOF'
set -euo pipefail
mount -t tmpfs tmpfs /sys/firmware/efi/efivars
if nixosctl secure-boot disable; then exit 1; fi
if NIXOS_NO_CHECK=1 "$1/bin/switch-to-configuration" boot; then exit 1; fi
EOF
cmp config.json /var/tmp/config-before-downgrade.json
test "$before" = "$(boot_files)"
echo secure-boot-downgrade-blocked
