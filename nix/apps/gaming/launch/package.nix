{ pkgs, cpuAffinity ? null }:
pkgs.writeShellApplication {
  name = "game-run";
  runtimeInputs = [ pkgs.util-linux ];
  text = ''
    if [ "$#" -eq 0 ]; then
      echo "Usage: game-run COMMAND [ARGUMENTS...]" >&2
      exit 2
    fi
    affinity="''${GAME_CPU_AFFINITY-${if cpuAffinity == null then "" else cpuAffinity}}"
    if [ -z "$affinity" ] || [ "$affinity" = all ]; then
      exec "$@"
    fi
    exec taskset --cpu-list -- "$affinity" "$@"
  '';
}
