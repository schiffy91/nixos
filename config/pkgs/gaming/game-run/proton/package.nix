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
    exec "$@"
  '';
}
