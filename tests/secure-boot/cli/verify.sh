set -euo pipefail
root=$(mktemp -d)
trap 'rm -rf "$root"' EXIT
mkdir -p "$root/bin" "$root/config"
printf '{"target":"Secure-Boot","host_path":"/etc/nixos/config/hosts/x86_64/STEAM-DECK-OLED/default.nix"}' > "$root/config/config.json"
cat > "$root/bin/nix" <<'STUB'
#!/bin/sh
touch "$VERIFY_ROOT/evaluated"
exit 91
STUB
chmod +x "$root/bin/nix"
export PATH="$root/bin:$PATH" VERIFY_ROOT="$root" NIXOS_CONFIG_ROOT="$root/config" NIXOSCTL_ASSUME_ROOT_FOR_TESTS=true
./build/nixosctl --help > "$root/help"
for action in prepare enable disable status; do grep -F "$action" "$root/help" >/dev/null; done
for arguments in 'secure-boot unexpected' 'secure-boot enable --microsoft' 'secure-boot disable --microsoft' 'secure-boot prepare --unexpected'; do
  read -r -a words <<< "$arguments"
  if ./build/nixosctl "${words[@]}" > "$root/result" 2>&1; then
    cat "$root/result"
    echo "CLI incorrectly accepted $arguments" >&2
    exit 1
  fi
  test ! -e "$root/evaluated"
done
echo secure-boot-cli-ok
