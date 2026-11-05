#!/usr/bin/env bash
set -u
work="$1"; passphrase="$2"; deadline_seconds="${3:-1200}"
log="$work/serial.log"; qmp="$work/qmp.sock"

send_key() {
  printf '{"execute":"qmp_capabilities"}\n{"execute":"send-key","arguments":{"keys":[{"type":"qcode","data":"%s"}],"hold-time":100}}\n' "$1" \
    | timeout 10 socat - "UNIX-CONNECT:$qmp" > /dev/null 2>&1
}

type_passphrase() {
  local index character
  for (( index = 0; index < ${#passphrase}; index++ )); do
    character="${passphrase:index:1}"
    send_key "$character"
    sleep 0.2
  done
  send_key ret
}

started=$SECONDS
attempt=0
next_attempt=$SECONDS   # modulo would skip; sleep never lands on it
while (( SECONDS - started < deadline_seconds )); do
  if grep -qa 'E2E-HEALTH-END' "$log" 2>/dev/null; then
    printf 'unlocked-and-reported'
    exit 0
  fi
  if (( SECONDS >= next_attempt )); then
    attempt=$(( attempt + 1 ))
    echo "typing the passphrase (attempt $attempt, ${SECONDS}s)" >&2
    type_passphrase
    next_attempt=$(( SECONDS + 120 ))
  fi
  sleep 5
done

echo "no health report after ${deadline_seconds}s" >&2
tail -c 2000 "$log" >&2
exit 1
