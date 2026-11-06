{ plymouth, libinput, systemd }:
(plymouth.override { inherit systemd; }).overrideAttrs (old: {
  patches = (old.patches or [ ]) ++ [ ./keyboard.patch ];
  buildInputs = old.buildInputs ++ [ libinput ];
  passthru = (old.passthru or { }) // { touchKeyboard = true; };
  postPatch = (old.postPatch or "") + ''
    cp ${./ply-osk.c} src/plugins/splash/two-step/ply-osk.c
    cp ${./ply-osk.h} src/plugins/splash/two-step/ply-osk.h
    cp ${./osk-layout.c} src/plugins/splash/two-step/osk-layout.c
    cp ${./osk-layout.h} src/plugins/splash/two-step/osk-layout.h
  '';
  doCheck = true;
  checkPhase = ''
    runHook preCheck
    $CC -std=c11 -Wall -Wextra -Werror \
      -I ../src/plugins/splash/two-step \
      ${../../../../../tests/plymouth-keyboard/model.c} \
      ../src/plugins/splash/two-step/osk-layout.c -lm -o osk-test
    ./osk-test
    runHook postCheck
  '';
})
