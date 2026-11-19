{ pkgs }:
pkgs.runCommand "proton-dead-links-tests" {} ''
  mkdir -p tools/kept-folder "other tools"
  ln -s /nix/store/00000000000000000000000000000000-proton-gone tools/dead
  ln -s /nix/store/00000000000000000000000000000000-proton-gone "other tools/dead too"
  ln -s ${pkgs.coreutils} tools/live
  ln -s /nowhere/by-hand tools/by-hand
  touch tools/kept-file
  said=$(${import ../../config/pkgs/gaming/proton/dead-links.nix { inherit pkgs; }} "$PWD/tools" "$PWD/other tools" "$PWD/absent")
  test "$(ls tools | tr '\n' ' ')" = "by-hand kept-file kept-folder live "
  test -z "$(ls "other tools")"
  test -L tools/by-hand
  test "$(printf '%s\n' "$said" | wc -l)" = 2
  touch $out
''
