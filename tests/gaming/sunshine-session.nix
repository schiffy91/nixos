{ pkgs }:
let
  systemctl = pkgs.writeShellScriptBin "systemctl" ''
    test "$*" = "--user is-active --quiet gamescope-session.service"
    exit "''${GAMESCOPE_STATUS:-3}"
  '';
  launcher = import ../../nix/apps/gaming/sunshine/session/package.nix {
    pkgs = pkgs // { systemd = systemctl; };
    desktopCommand = "${pkgs.coreutils}/bin/echo desktop";
    gamingCommand = "${pkgs.coreutils}/bin/echo gaming";
  };
in pkgs.runCommand "sunshine-session-tests" {} ''
  launcher=${launcher}/bin/sunshine-session
  test "$(XDG_CURRENT_DESKTOP=KDE $launcher)" = desktop
  test "$(XDG_CURRENT_DESKTOP=another:KDE $launcher)" = desktop
  test "$(XDG_CURRENT_DESKTOP=gamescope GAMESCOPE_STATUS=0 $launcher)" = gaming
  # A previous Plasma session can leave KDE in the user manager environment.
  test "$(XDG_CURRENT_DESKTOP=KDE GAMESCOPE_STATUS=0 $launcher)" = gaming
  if XDG_CURRENT_DESKTOP=unknown $launcher; then
    echo "Unknown session unexpectedly launched Sunshine" >&2
    exit 1
  fi
  touch $out
''
