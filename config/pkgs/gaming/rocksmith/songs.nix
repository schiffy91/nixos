{ pkgs }:
# The game reads one folder. Each song is a link there to where it is kept, so one copy serves the game, the
# Drive and every other machine; a file the game or Steam put there itself is never replaced.
pkgs.writeShellApplication {
  name = "rocksmith-songs";
  runtimeInputs = with pkgs; [ coreutils findutils util-linux ];
  text = ''
    game=$1 songs=$2 volume=''${3:-}
    if ! [ -d "$game" ] || ! [ -d "$songs" ]; then exit 0; fi  # no game here, or no songs
    if [ -n "$volume" ] && ! mountpoint -q "$volume"; then exit 0; fi  # the songs' volume is away: links stay
    find "$game" -maxdepth 1 -xtype l -delete  # a link whose song is gone
    while IFS= read -r -d "" song; do
      link="$game/''${song##*/}"
      if [[ $link -ef $song ]] || [[ -e $link && ! -L $link ]]; then continue; fi  # linked already, or the game's own file
      ln -sfn "$song" "$link"
    done < <(find "$songs" -type f -iname '*.psarc' -print0)
  '';
}
