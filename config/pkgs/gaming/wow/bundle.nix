{ pkgs, addons }:
pkgs.runCommand "wow-addons" {} ''
  mkdir -p "$out"
  ${pkgs.lib.concatMapStringsSep "\n" (package: ''
    found=false
    for addon in ${package}/*; do
      [ -d "$addon" ] || continue
      found=true
      # Two packages must not silently merge or replace the same addon.
      ln -sT "$addon" "$out/$(basename "$addon")"
    done
    if [ "$found" = false ]; then
      echo "No addon directories in ${package}" >&2
      exit 1
    fi
  '') addons}
''
