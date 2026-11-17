#!/usr/bin/env bash
# Screenshots the VM's panel until boot-evidence accepts it, instead of sleeping a fixed time and
# looking once. Usage: wait_evidence.sh <work directory> <seconds> <screenshot name> <settled polls>
# <boot-evidence arguments, with @ where the screenshot goes>
#
# The unlock agent starts on the firmware's framebuffer and is redrawn when the GPU driver takes the
# panel over, at another resolution. A screenshot is taken as the panel only once <settled polls>
# accepted screenshots in a row, two seconds apart, have the same size.
set -u
work=$1; deadline_seconds=$2; name=$3; settled=$4
shift 4
qmp="$work/qmp.sock"; shot="$work/$name.ppm"
verdict='no screenshot'
size=''; same=0
started=$SECONDS
while (( SECONDS - started < deadline_seconds )); do
  printf '{"execute":"qmp_capabilities"}\n{"execute":"screendump","arguments":{"filename":"%s"}}\n' "$shot" \
    | timeout 10 socat - "UNIX-CONNECT:$qmp" > /dev/null 2>&1
  if [ -s "$shot" ] && verdict=$(./build/boot-evidence "${@/#@/$shot}" 2>&1); then
    now=$(head -c 64 "$shot" | sed -n 2p)  # a PPM's second line is its width and height
    if [ "$now" = "$size" ]; then same=$((same + 1)); else size=$now; same=1; fi
    if (( same >= settled )); then
      printf '%s\n' "$verdict"
      exit 0
    fi
  else
    same=0
  fi
  sleep 2
done
echo "the panel was not accepted within ${deadline_seconds}s: $verdict" >&2
exit 1
