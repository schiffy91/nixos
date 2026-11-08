{ plymouth, lib, libinput, systemd }:
(plymouth.override { inherit systemd; }).overrideAttrs (old: {
  version = "26.310.630-osk";
  src = builtins.fetchTree {
    type = "git";
    url = "https://gitlab.freedesktop.org/plymouth/plymouth.git";
    rev = "b79b178641e41682204052036da6591896097473";
    narHash = "sha256-s/jMY0Pydh4XrIO7JohFOGBIvd1FdftraLILdWKEgT0=";
  };
  patches = (old.patches or [ ]) ++ map
    (name: ./patches + "/${name}")
    (builtins.filter (lib.hasSuffix ".patch") (builtins.attrNames (builtins.readDir ./patches)));
  buildInputs = old.buildInputs ++ [ libinput ];
  mesonFlags = old.mesonFlags ++ [ "-Dtouchscreen=enabled" ];
  passthru = (old.passthru or { }) // { touchKeyboard = true; };
  postPatch = lib.replaceStrings [ "'26.134.222'" ]
    [ "'26.310.630-osk'" ] (old.postPatch or "");
  doCheck = true;
  checkPhase = ''
    runHook preCheck
    meson test --no-rebuild --print-errorlogs --suite unit --suite fuzz
    runHook postCheck
  '';
})
