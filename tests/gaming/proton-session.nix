{ pkgs }:
let
  systemctl = pkgs.writeShellScriptBin "systemctl" ''
    test "$*" = "--user is-active --quiet gamescope-session.service"
    exit "''${GAMESCOPE_STATUS:-3}"
  '';
  launcher = import ../../config/pkgs/gaming/game-run/proton/package.nix {
    pkgs = pkgs // { systemd = systemctl; };
  };
  capture = pkgs.writeShellScript "capture-proton-driver" ''
    printf '%s\n' "$PROTON_ENABLE_WAYLAND" "$@"
  '';
  steamInput = pkgs.writeShellScript "capture-steam-input" ''
    printf '%s\n' "''${PROTON_NO_STEAMINPUT-unset}"
  '';
in pkgs.runCommand "proton-session-tests" {} ''
  launcher=${launcher}/bin/proton-session
  export WAYLAND_DISPLAY=wayland-0
  test "$(XDG_CURRENT_DESKTOP=KDE $launcher ${capture})" = 1
  test "$(XDG_CURRENT_DESKTOP=gamescope $launcher ${capture})" = 0
  test "$(XDG_CURRENT_DESKTOP=KDE GAMESCOPE_STATUS=0 $launcher ${capture})" = 0
  test "$(WAYLAND_DISPLAY= $launcher ${capture})" = 0
  test "$(PROTON_ENABLE_WAYLAND=0 $launcher ${capture})" = 0
  test "$(PROTON_ENABLE_WAYLAND=1 GAMESCOPE_STATUS=0 $launcher ${capture})" = 1
  test "$(XDG_CURRENT_DESKTOP=KDE $launcher ${capture} 'argument with spaces')" = "$(printf '1\nargument with spaces')"
  # Steam's virtual gamepad: kept for a game started outside Steam, left to
  # Proton's own choice in one Steam started, and never overriding the caller.
  test "$($launcher ${steamInput})" = 0
  test "$(SteamGameId=1234 $launcher ${steamInput})" = unset
  test "$(PROTON_NO_STEAMINPUT=1 $launcher ${steamInput})" = 1
  touch $out
''
