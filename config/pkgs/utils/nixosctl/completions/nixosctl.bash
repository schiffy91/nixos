_nixosctl() {
  local IFS=$'\n'
  COMPREPLY=($(nixosctl complete "${COMP_WORDS[@]:1:COMP_CWORD}" 2>/dev/null | cut -f1))
}
complete -o default -F _nixosctl nixosctl
