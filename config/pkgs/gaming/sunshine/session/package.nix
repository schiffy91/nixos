{ pkgs, desktopCommand, gamingCommand }:
pkgs.writeShellApplication {
  name = "sunshine-session";
  runtimeInputs = [ pkgs.systemd ];
  text = ''
    # Jovian starts gamescope before graphical-session.target. Query its unit:
    # the user manager can retain KDE's environment after switching sessions.
    if systemctl --user is-active --quiet gamescope-session.service; then
      exec ${gamingCommand}
    fi
    case ":''${XDG_CURRENT_DESKTOP:-}:" in
      *:KDE:*|*:plasma:*) exec ${desktopCommand} ;;
      *) echo "Sunshine: no supported graphical session is active" >&2; exit 1 ;;
    esac
  '';
}
