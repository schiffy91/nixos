{ pkgs, songs ? "${import ../../config/pkgs/gaming/rocksmith/songs.nix { inherit pkgs; }}/bin/rocksmith-songs" }:
pkgs.runCommand "rocksmith-songs-check" {} ''
  no() { echo "rocksmith-songs: $1" >&2; exit 1; }
  mkdir -p game library/dlc library/cdlc/pack
  echo official > library/dlc/a_p.psarc; echo custom > library/cdlc/pack/b_p.PSARC; echo notes > library/readme.txt
  echo "the game's own" > game/a_p.psarc; ln -s "$PWD/library/gone_p.psarc" game/gone_p.psarc
  ${songs} "$PWD/game" "$PWD/library"
  [ ! -L game/a_p.psarc ] || no "a file of the game's own was replaced"
  [ "$(cat game/a_p.psarc)" = "the game's own" ] || no "a file of the game's own was changed"
  [ "$(readlink game/b_p.PSARC)" = "$PWD/library/cdlc/pack/b_p.PSARC" ] || no "a song below the top folder is not linked"
  [ ! -e game/readme.txt ] || no "something that is not a song was linked"
  [ ! -L game/gone_p.psarc ] || no "a link to a song that is gone stayed"
  rm game/a_p.psarc; before=$(stat -c %Y:%i game/b_p.PSARC)
  ${songs} "$PWD/game" "$PWD/library"
  [ "$(readlink game/a_p.psarc)" = "$PWD/library/dlc/a_p.psarc" ] || no "a song the game has no file for is not linked"
  [ "$(stat -c %Y:%i game/b_p.PSARC)" = "$before" ] || no "a second run made a link again"
  rm library/dlc/a_p.psarc
  ${songs} "$PWD/game" "$PWD/library" "$PWD/library"
  [ -L game/a_p.psarc ] || no "links were removed while the songs' volume was not mounted"
  ${songs} "$PWD/game" "$PWD/library"
  [ ! -L game/a_p.psarc ] || no "a link to a song that is gone stayed"
  [ -L game/b_p.PSARC ] || no "a link to a song that is there was removed"
  ${songs} "$PWD/missing" "$PWD/library" || no "a machine without the game is an error"
  ${songs} "$PWD/game" "$PWD/missing" || no "a machine without songs is an error"
  touch "$out"
''
