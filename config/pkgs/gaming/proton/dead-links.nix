# Removes, from the folders given, each link into the Nix store whose build the store no longer has: a
# tool that Steam lists and cannot run. A folder, a file, a live link or a link elsewhere is left alone.
{ pkgs }: pkgs.writeShellScript "remove-dead-store-links" ''
  for folder in "$@"; do
    for link in "$folder"/*; do
      [ -L "$link" ] && [ ! -e "$link" ] || continue
      case "$(${pkgs.coreutils}/bin/readlink "$link")" in
        /nix/store/*) ${pkgs.coreutils}/bin/rm -f -- "$link" && echo "removed the dead link $link" ;;
      esac
    done
  done
''
