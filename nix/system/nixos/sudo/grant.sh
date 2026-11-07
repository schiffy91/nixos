# Included by sudo.nix with the configured administrator and a trusted PATH.
usage() {
  echo 'Usage: nixosctl sudo enable --for <duration> | status | disable' >&2
  echo 'Duration: 1s to 24h, using an integer followed by s, m, or h.' >&2
  exit 2
}

action=${1:-}
case "$action" in
  enable)
    [[ $# == 3 && $2 == --for && $3 =~ ^([1-9][0-9]{0,4})(s|m|h)$ ]] || usage
    duration=${BASH_REMATCH[1]}
    case "${BASH_REMATCH[2]}" in
      m) duration=$((duration * 60)) ;;
      h) duration=$((duration * 3600)) ;;
    esac
    ((duration <= 86400)) || usage
    ;;
  status|disable) [[ $# == 1 ]] || usage ;;
  *) usage ;;
esac

if [[ $EUID != 0 ]]; then
  if [[ $action == status ]]; then
    exec /run/wrappers/bin/sudo -n -- /run/current-system/sw/bin/nixosctl-sudo status
  fi
  # -k plus the final PASSWD rule requires ordinary authentication even when
  # a grant is active. Use the installed helper, never a caller-supplied path.
  exec /run/wrappers/bin/sudo -k -- /run/current-system/sw/bin/nixosctl-sudo "$@"
fi

directory=/run/nixosctl-sudo
grant=$directory/grant
if [[ ! -e $directory && ! -L $directory ]]; then
  if [[ $action == status ]]; then
    echo "No temporary sudo grant for $admin."
    exit 0
  fi
  mkdir -m 0755 "$directory"
fi
if [[ -L $directory || ! -d $directory || $(stat -c '%u:%g:%a' "$directory") != 0:0:755 ]]; then
  echo "Refusing unsafe sudo grant directory: $directory" >&2
  exit 1
fi

if [[ $action == status ]]; then
  if [[ ! -e $grant && ! -L $grant ]]; then
    echo "No temporary sudo grant for $admin."
    exit 0
  fi
  if [[ -L $grant || ! -f $grant || $(stat -c '%u:%g:%a' "$grant") != 0:0:440 ]]; then
    echo 'Refusing unsafe sudo grant file.' >&2
    exit 1
  fi
  read -r marker expires < "$grant"
  [[ $marker == '#expires' && $expires =~ ^[0-9]+$ ]] || exit 1
  if (( $(date +%s) >= expires )); then
    echo "Temporary sudo grant for $admin has expired."
  else
    echo "Temporary sudo grant for $admin expires $(date -u -d "@$expires" '+%Y-%m-%d %H:%M:%S UTC')."
  fi
  exit 0
fi

# The parent is root-controlled. Dotfiles are ignored by sudo's includedir.
umask 077
exec 9> "$directory/.lock"
flock -x 9
if [[ $action == disable ]]; then
  rm -f -- "$grant"
  echo "Temporary sudo grant for $admin disabled. Existing root processes continue."
  exit 0
fi

expires=$(( $(date +%s) + duration ))
timestamp=$(date -u -d "@$expires" '+%Y%m%d%H%M%SZ')
temporary=$(mktemp "$directory/.grant.XXXXXXXX")
trap 'rm -f -- "$temporary"' EXIT
printf '#expires %s\n%s ALL=(ALL:ALL) NOTAFTER=%s NOPASSWD: ALL\n' "$expires" "$admin" "$timestamp" > "$temporary"
@visudo@ -cf "$temporary" >/dev/null
chmod 0440 "$temporary"
mv -fT -- "$temporary" "$grant"
echo "Temporary sudo grant for $admin expires $(date -u -d "@$expires" '+%Y-%m-%d %H:%M:%S UTC')."
echo 'This applies to every process using this account; it is not an agent sandbox.'
echo 'Expiration does not stop existing root processes or undo their changes.'
