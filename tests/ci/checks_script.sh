#!/usr/bin/env bash
# ci/checks.sh against a pretend nix: an unreadable or empty check list fails, every check is built
# in one invocation, and a failing check is named.
set -euo pipefail

root=$(cd "$(dirname "$0")/../.." && pwd)
script=${CHECKS_SCRIPT:-$root/ci/checks.sh}
temporary=$(mktemp -d)
trap 'rm -rf "$temporary"' EXIT

mkdir -p "$temporary/bin" "$temporary/repo/ci"
cp "$script" "$temporary/repo/ci/checks.sh"
cat > "$temporary/bin/nix" <<'EOF'
#!/usr/bin/env bash
printf '%s\n' "$*" >> "$FAKE_NIX_LOG"
case "$1" in
  eval)
    case "$*" in
      *currentSystem*) printf 'test-system' ;;
      *) [ "$FAKE_NIX_LIST" = fail ] && exit 1; printf '%s' "$FAKE_NIX_LIST" ;;
    esac ;;
  build)
    for argument in "$@"; do
      case "$argument" in *"#checks.test-system.$FAKE_NIX_BROKEN") exit 1 ;; esac
    done ;;
esac
EOF
chmod +x "$temporary/bin/nix"

run() {  # run <list> <broken check>: the script's exit status, with its output and nix's calls kept
    : > "$temporary/log"
    status=0
    PATH="$temporary/bin:$PATH" FAKE_NIX_LOG="$temporary/log" FAKE_NIX_LIST="$1" FAKE_NIX_BROKEN="$2" \
        bash "$temporary/repo/ci/checks.sh" > "$temporary/output" 2>&1 || status=$?
}

run fail none
[[ $status -ne 0 ]] || { echo "a check list that cannot be read passed"; exit 1; }
grep -q 'cannot list the checks' "$temporary/output"
if grep -q '^build' "$temporary/log"; then echo "built without a list"; exit 1; fi

run '' none
[[ $status -ne 0 ]] || { echo "an empty check list passed"; exit 1; }
grep -q 'no checks are listed' "$temporary/output"

run $'alpha\nbeta\ngamma' none
[[ $status -eq 0 ]] || { echo "three passing checks failed"; cat "$temporary/output"; exit 1; }
[[ $(grep -c '^build' "$temporary/log") -eq 1 ]] || { echo "more than one build for passing checks"; cat "$temporary/log"; exit 1; }
for name in alpha beta gamma; do grep '^build' "$temporary/log" | grep -q "#checks.test-system.$name"; done

run $'alpha\nbeta\ngamma' beta
[[ $status -ne 0 ]] || { echo "a failing check passed"; exit 1; }
grep -qx 'failed: beta' "$temporary/output" || { echo "the failing check was not named"; cat "$temporary/output"; exit 1; }

echo "PASS: ci/checks.sh fails without a list, builds once, and names what failed"
