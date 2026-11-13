{ pkgs, inputs }:
let
  programs = import ../../../config/pkgs/gaming/deckpad/package.nix { inherit pkgs inputs; };
  tests = (import ../../../config/lib/btrc.nix { inherit pkgs inputs; }) {
    name = "deckpad-tests";
    src = pkgs.lib.fileset.toSource {
      root = ../../..;
      fileset = pkgs.lib.fileset.unions [
        ./UnitTests.btrc ./ServerTests.btrc ./btrc.toml ./btrc.lock ../../lib/Check.btrc
        ../../../config/lib/BackgroundProcess.btrc
        (pkgs.lib.fileset.fileFilter (file: file.hasExt "btrc" || file.hasExt "h" || file.name == "btrc.toml" || file.name == "btrc.lock") ../../../config/pkgs/gaming/deckpad)
      ];
    };
    programs = {
      deckpad-unit-tests = "tests/gaming/deckpad/UnitTests.btrc";
      deckpad-server-tests = "tests/gaming/deckpad/ServerTests.btrc";
    };
    buildInputs = [ pkgs.dbus pkgs.libsodium ];
    env.NIX_CFLAGS_COMPILE = "-I${pkgs.lib.getDev pkgs.dbus}/include/dbus-1.0 -I${pkgs.lib.getLib pkgs.dbus}/lib/dbus-1.0/include";
    linkFlags = [ "-ldbus-1" "-lsodium" ];
  };
# The server test is a host for the real program over loopback: it pairs,
# attaches through the encrypted tunnel and drives the controller with a
# USB/IP host of its own. The kernel's side of each link is in kernel.nix.
#
# The screen is shown on a virtual display, beside a pretend service (the
# files it leaves) and a pretend backlight.
in pkgs.runCommand "deckpad-check" { nativeBuildInputs = [ pkgs.xvfb pkgs.xdotool pkgs.imagemagick ]; } ''
  ${tests}/bin/deckpad-unit-tests
  ${tests}/bin/deckpad-server-tests ${programs.deckpad}/bin/deckpad
  ${programs.tray}/bin/deckpad-tray

  mkdir -p run panels/panel session bin
  printf '#!/bin/sh\necho "$*" >> %s/stopped\n' "$PWD" > bin/systemctl
  chmod +x bin/systemctl
  export PATH=$PWD/bin:$PATH DECKPAD_RUNTIME=$PWD/run DECKPAD_BACKLIGHTS=$PWD/panels XDG_RUNTIME_DIR=$PWD/session DISPLAY=:99
  Xvfb :99 -screen 0 1280x800x24 -nolisten tcp > /dev/null 2>&1 &
  sleep 2
  lit() { test "$(import -window root gray:- | tr -d '\0' | wc -c)" -gt 0; }
  level() { cat panels/panel/brightness; }
  fail() { echo "FAIL: deckpad-screen: $1"; exit 1; }

  echo token > run/token
  echo 20347 > panels/panel/brightness
  touch run/usb-host
  ${programs.screen}/bin/deckpad-screen &
  screen=$!
  sleep 2
  lit && [ "$(level)" = 20347 ] || fail "a host's notice shows at the panel's own brightness first"
  sleep 6
  ! lit && [ "$(level)" = 0 ] || fail "then the screen is black at the lowest brightness"
  xdotool mousemove 640 400 click 1
  sleep 1
  lit && [ "$(level)" = 20347 ] && kill -0 $screen || fail "a touch while a host holds the controller shows the notice again and stops nothing"
  sleep 7
  rm run/usb-host
  sleep 1
  lit && [ "$(level)" = 20347 ] || fail "the notice returns when the host leaves"
  xdotool mousemove 640 400 click 1
  sleep 2
  ! kill -0 $screen 2> /dev/null && grep -q "stop deckpad.service" stopped || fail "a touch while no host holds the controller stops the service"

  rm stopped
  touch run/usb-host
  ${programs.screen}/bin/deckpad-screen &
  screen=$!
  sleep 8
  [ "$(level)" = 0 ] || fail "the screen is dark again on the next start"
  rm run/token run/usb-host
  sleep 5
  ! kill -0 $screen 2> /dev/null && [ "$(level)" = 20347 ] && [ ! -e stopped ] || fail "the screen ends with the service and gives the brightness back"

  echo 0 > panels/panel/brightness
  echo 31000 > session/deckpad-screen-brightness
  echo token > run/token
  ${programs.screen}/bin/deckpad-screen &
  screen=$!
  sleep 2
  [ "$(level)" = 31000 ] || fail "a brightness left dark by a killed screen is put back"
  kill -TERM $screen
  sleep 2
  ! kill -0 $screen 2> /dev/null && grep -q "stop deckpad.service" stopped || fail "a stop signal ends the screen and the service"
  echo "PASS: deckpad-screen notices, darkness, touch, brightness and stopping on a virtual display"
  touch "$out"
''
