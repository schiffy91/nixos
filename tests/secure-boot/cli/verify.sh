set -euo pipefail
root=$(mktemp -d)
trap 'rm -rf "$root"' EXIT
mkdir -p "$root/bin" "$root/boot/EFI/Linux" "$root/config"
printf '{"target":"Secure-Boot","host_path":"/etc/nixos/config/hosts/x86_64/VM-TEST/default.nix"}' > "$root/config/config.json"
cat > "$root/bin/nix" <<'EOF'
#!/bin/sh
case "$*" in
  *efiSysMountPoint*) printf %s "$VERIFY_ROOT/boot" ;;
  *efiArch*) printf x64 ;;
  *recovery.enable*) printf false ;;
  *) exit 1 ;;
esac
EOF
cat > "$root/bin/bootctl" <<'EOF'
#!/bin/sh
printf '[{"type":"type2","path":"%s/boot/EFI/Linux/default.efi","isDefault":true}]' "$VERIFY_ROOT"
EOF
cat > "$root/bin/sbctl" <<'EOF'
#!/bin/sh
if [ "$1" = status ]; then printf 'Secure Boot: disabled\n'; exit 0; fi
case "$VERIFY_CASE" in
  error) printf '[]'; exit 1 ;;
  empty) printf '[]' ;;
  malformed) printf '{' ;;
  old-schema) printf '{"/boot/good.efi":{"is_signed":true}}' ;;
  omitted) printf '[{"file_name":"/unrelated.efi","is_signed":1}]' ;;
  *)
    signed=1
    [ "$VERIFY_CASE" != unsigned ] || signed=0
    printf '[{"file_name":"%s/boot/EFI/BOOT/BOOTX64.EFI","is_signed":1},{"file_name":"%s/boot/EFI/systemd/systemd-bootx64.efi","is_signed":1},{"file_name":"%s/boot/EFI/Linux/default.efi","is_signed":%s}]' "$VERIFY_ROOT" "$VERIFY_ROOT" "$VERIFY_ROOT" "$signed"
    ;;
esac
EOF
chmod +x "$root/bin/"*
export PATH="$root/bin:$PATH" VERIFY_ROOT="$root" NIXOS_CONFIG_ROOT="$root/config" NIXOSCTL_ASSUME_ROOT_FOR_TESTS=true
for VERIFY_CASE in error empty malformed old-schema omitted unsigned; do
  export VERIFY_CASE
  if ./build/nixosctl secure-boot status > "$root/result" 2>&1; then
    cat "$root/result"
    echo "Verification incorrectly accepted $VERIFY_CASE" >&2
    exit 1
  fi
done
export VERIFY_CASE=signed
./build/nixosctl secure-boot status
echo secure-boot-cli-ok
