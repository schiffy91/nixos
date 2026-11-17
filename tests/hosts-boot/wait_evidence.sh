#!/usr/bin/env bash
# Screenshots the VM's panel until boot-evidence accepts it, instead of sleeping a fixed time and
# looking once. Usage: wait_evidence.sh <work directory> <seconds> <screenshot name> <boot-evidence
# arguments, with @ where the screenshot goes>
set -u
work=$1; deadline_seconds=$2; name=$3
shift 3
qmp="$work/qmp.sock"; shot="$work/$name.ppm"
verdict='no screenshot'

started=$SECONDS
while (( SECONDS - started < deadline_seconds )); do
  printf '{"execute":"qmp_capabilities"}\n{"execute":"screendump","arguments":{"filename":"%s"}}\n' "$shot" \
    | timeout 10 socat - "UNIX-CONNECT:$qmp" > /dev/null 2>&1
  if [ -s "$shot" ] && verdict=$(./build/boot-evidence "${@/#@/$shot}" 2>&1); then
    printf '%s\n' "$verdict"
    exit 0
  fi
  sleep 2
done

echo "the panel was not accepted within ${deadline_seconds}s: $verdict" >&2
exit 1
