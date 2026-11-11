# Battle.net shares its Wine server with games. Only refresh DPI when the
# prefix contains launcher/Windows infrastructure, never an active game.
prefix_processes() {
  local pid
  for pid in /proc/[0-9]*; do
    if [ "${pid##*/}" = "$$" ] || [ "${pid##*/}" = "$BASHPID" ]; then continue; fi
    if { tr '\0' '\n' < "$pid/environ"; } 2>/dev/null | grep -qxF \
      -e "WINEPREFIX=$prefix" -e "WINEPREFIX=$prefix/pfx" -e "WINEPREFIX=$prefix/pfx/" \
      -e "STEAM_COMPAT_DATA_PATH=$prefix" -e "STEAM_COMPAT_DATA_PATH=$prefix/pfx" -e "STEAM_COMPAT_DATA_PATH=$prefix/pfx/"; then
      printf '%s\n' "${pid##*/}"
    fi
  done
}

prefix_has_game() {
  local pid executable
  for pid in $(prefix_processes); do
    executable=''
    { IFS= read -r -d '' executable < "/proc/$pid/cmdline"; } 2>/dev/null || continue
    executable=${executable,,}
    executable=${executable//\\//}
    case "$executable" in
      */battle.net/battle.net*.exe|*/battle.net/agent/*/agent.exe|*/windows/system32/*.exe|*/windows/syswow64/*.exe) ;;
      services.exe|winedevice.exe|plugplay.exe|explorer.exe|rpcss.exe|svchost.exe|conhost.exe) ;;
      *.exe) return 0 ;;
    esac
  done
  return 1
}

stop_prefix() {
  local pid attempt
  for pid in $(prefix_processes); do kill "$pid" 2>/dev/null || true; done
  for ((attempt=0; attempt<50; attempt++)); do
    if [ -z "$(prefix_processes)" ]; then return 0; fi
    sleep 0.1
  done
  printf '%s\n' 'Battle.net is still closing; leaving its registry untouched.' >&2
  return 1
}

follow_display() {
  local fallback=$1 dpi current child status refresh session_lock
  shift
  exec {session_lock}> "$prefix/.battlenet-session.lock"
  if ! flock -n "$session_lock"; then
    notify-send Battle.net 'Battle.net is already running.' || true
    return 0
  fi
  if prefix_has_game; then
    notify-send Battle.net 'A game is running. Close it before relaunching Battle.net.' || true
    return 1
  fi
  stop_prefix || return 1
  while true; do
    dpi=$(wine-prefix-dpi display --dpi "$fallback")
    wine-prefix-dpi set --dpi "$dpi" -- "$prefix"
    "$@" &
    child=$!
    refresh=false
    while kill -0 "$child" 2>/dev/null; do
      sleep 3
      if ! kill -0 "$child" 2>/dev/null; then break; fi
      current=$(wine-prefix-dpi display --dpi "$dpi")
      if [ "$current" != "$dpi" ] && ! prefix_has_game; then
        # Recheck after the display settles; hotplug can report intermediate states.
        sleep 1
        if [ "$(wine-prefix-dpi display --dpi "$dpi")" != "$current" ] || prefix_has_game; then continue; fi
        printf 'Display DPI changed from %s to %s; refreshing Battle.net.\n' "$dpi" "$current"
        stop_prefix || return 1
        wait "$child" || true
        refresh=true
        break
      fi
    done
    if ! "$refresh"; then
      status=0
      wait "$child" || status=$?
      return "$status"
    fi
  done
}
