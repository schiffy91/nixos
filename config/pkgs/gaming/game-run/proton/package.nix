{ pkgs }:
pkgs.writeShellApplication {
  name = "proton-session";
  runtimeInputs = [ pkgs.systemd ];
  text = ''
    # Gamescope lacks wl_subcompositor, which Wine's Wayland driver requires.
    # The unit check also handles stale Plasma environment after a session switch.
    wayland=0
    if ! systemctl --user is-active --quiet gamescope-session.service 2>/dev/null; then
      case ":''${XDG_CURRENT_DESKTOP:-}:" in
        *:gamescope:*) ;;
        *) if [ -n "''${WAYLAND_DISPLAY:-}" ]; then wayland=1; fi ;;
      esac
    fi
    export PROTON_ENABLE_WAYLAND="''${PROTON_ENABLE_WAYLAND:-$wayland}"
    # Proton reads Steam's virtual gamepad only in games Steam started, and
    # ignores it in the rest. A Steam Controller, or a Deck serving as one, has
    # no other gamepad on a PC, so a game started outside Steam reads that pad
    # too, beside the controllers plugged in.
    if [ -z "''${SteamGameId:-}" ]; then
      export PROTON_NO_STEAMINPUT="''${PROTON_NO_STEAMINPUT:-0}"
    fi
    exec "$@"
  '';
}
