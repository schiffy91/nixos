{ pkgs }:
let
  ready = pkgs.writeShellScript "ready" ''echo "ready $*" >> "$LOG"'';
  whenClosed = import ../../config/pkgs/gaming/steam/when-closed.nix { inherit pkgs; user = "$(id -u)"; ready = "${ready}"; };
  script = whenClosed "test" ''echo written >> "$LOG"'';
in pkgs.runCommand "steam-when-closed-tests" { nativeBuildInputs = [ pkgs.procps ]; } ''
  export LOG=$PWD/log
  # Steam closed: the work is done, and only then is the unit told it is ready.
  ${script}
  test "$(cat $LOG)" = "$(printf 'written\nready --status=done')"

  # Steam open: the unit is told at once, and the work waits for the last Steam process to exit.
  : > $LOG
  printf '#!${pkgs.runtimeShell}\nread -r line < "$1"\n' > steam
  chmod +x steam
  mkfifo first second
  ./steam first &
  ./steam second &
  for attempt in $(seq 50); do pgrep -x steam > /dev/null && break; sleep 0.1; done
  ${script} &
  waiter=$!
  for attempt in $(seq 50); do test -s $LOG && break; sleep 0.1; done
  test "$(cat $LOG)" = "ready --status=waiting for Steam to close"
  echo > first
  sleep 1
  test "$(cat $LOG)" = "ready --status=waiting for Steam to close"  # one Steam process is still there
  echo > second
  wait $waiter
  test "$(cat $LOG)" = "$(printf 'ready --status=waiting for Steam to close\nwritten')"
  touch $out
''
