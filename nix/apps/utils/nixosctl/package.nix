{ pkgs, lib, btrcpy, configPath ? "" }:
let secureBootKeys = import ./secure-boot-keys/package.nix { inherit pkgs; };
in pkgs.stdenv.mkDerivation {
  name = "nixosctl";
  src = lib.fileset.toSource {
    root = ../../../..;
    fileset = lib.fileset.unions [ ../../../../btrc ../../../../tests/e2e ];
  };
  nativeBuildInputs = [ btrcpy pkgs.makeWrapper ];
  dontConfigure = true;
  buildPhase = ''
    export HOME="$TMPDIR"  # btrcpy writes its cache under $HOME
    btrcpy --strict-imports btrc/nixosctl/NixosCtl.btrc -o nixosctl.c
    $CC -std=c11 -O2 nixosctl.c -lm -lpthread -lutil -o nixosctl
    btrcpy --strict-imports btrc/nixosctl/PasswordHashFileMain.btrc -o password-file.c
    $CC -std=c11 -O2 password-file.c -lm -lpthread -lutil -o nixosctl-password-file
  '';
  installPhase = ''
    mkdir -p $out/bin
    cp nixosctl $out/bin/nixosctl
    cp nixosctl-password-file $out/bin/nixosctl-password-file
    ln -s ${secureBootKeys}/bin/nixosctl-secure-boot-keys $out/bin/nixosctl-secure-boot-keys
    wrapProgram $out/bin/nixosctl \
      ${lib.optionalString (configPath != "") "--set NIXOS_CONFIG ${lib.escapeShellArg configPath}"} \
      --prefix PATH : ${lib.makeBinPath (with pkgs; [
        secureBootKeys
        kdePackages.libkscreen
        kdePackages.konsole
        systemd
        power-profiles-daemon
        pulseaudio
        sbctl
        sbsigntool
        btrfs-progs
        cryptsetup
        util-linux
        coreutils
        git
        mkpasswd
        nix
      ])}
  '';
}
