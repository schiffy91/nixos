#!/usr/bin/env bash
set -euo pipefail
source "$1"
prefix=$(mktemp -d)
launcher_pid=''
game_pid=''
trap 'kill ${launcher_pid:-} ${game_pid:-} 2>/dev/null || true; rm -rf "$prefix"' EXIT
notify-send() { :; }

env WINEPREFIX="$prefix/pfx" bash -c 'exec -a "C:\Program Files (x86)\Battle.net\Battle.net.exe" bash -c "sleep 60; :"' &
launcher_pid=$!
sleep 0.1
if prefix_has_game; then echo 'Launcher mistaken for a game' >&2; exit 1; fi

# An unfamiliar executable must block restart as reliably as Wow.exe does.
env WINEPREFIX="$prefix/pfx" bash -c 'exec -a "C:\Games\UnknownGame.exe" bash -c "sleep 60; :"' &
game_pid=$!
sleep 0.1
prefix_has_game
if (follow_display 96 false); then echo 'Active game did not block relaunch' >&2; exit 1; fi
kill -0 "$game_pid"
kill -0 "$launcher_pid"
kill "$game_pid"
wait "$game_pid" || true
game_pid=''
stop_prefix
wait "$launcher_pid" || true
launcher_pid=''

# An immediately failing launcher must return its status, without a restart loop.
wine-prefix-dpi() { if [ "$1" = display ]; then echo 96; fi; }
status=0
(follow_display 96 bash -c 'exit 7') || status=$?
[ "$status" = 7 ]
echo 'PASS: real prefix process detection, active-game protection, shutdown and launcher exit status'
