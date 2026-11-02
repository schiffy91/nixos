# Allow patterns with no matches to pass through
setopt no_nomatch

print-files() {
  if [ -z "$1" ]; then
    echo "Error: Directory path is required"
    return 1
  fi
  local directory="$1"
  shift
  set -- "${@:-*}"
  for pattern in "$@"; do
    find "$directory" -name "$pattern" -type f -print0 | while IFS= read -r -d '' file; do
      echo "==File: $file=="
      cat "$file"
      echo
    done
  done
}

extract-substring() {
  p="$1"
  s="$2"
  f="$3"
  c=$(cat "$f")
  ep=$(printf '%s\n' "$p" | sed 's/[\.*\/\[\]^$]/\\&/g')
  es=$(printf '%s\n' "$s" | sed 's/[\.*\/\[\]^$]/\\&/g')

  # Use sed for substring extraction in a POSIX-compliant way
  extracted=$(printf '%s\n' "$c" | sed -n "s/$ep\(.*\)$es/\\1/p")

  if [ -n "$extracted" ]; then
    printf '%s\n' "$extracted"
  else
    printf 'Error: Substring not found between prefix "%s" and suffix "%s".\n' "$p" "$s" >&2
    return 1
  fi
}

alias clauded="claude --dangerously-skip-permissions"
