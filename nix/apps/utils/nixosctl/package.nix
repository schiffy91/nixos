{ pkgs, lib, btrcpy, configPath ? "" }:
pkgs.stdenv.mkDerivation {
  name = "nixosctl";
  src = ../../../..;
  nativeBuildInputs = [ btrcpy pkgs.makeWrapper ];
  dontConfigure = true;
  buildPhase = ''
    export HOME="$TMPDIR"  # btrcpy writes its cache under $HOME
    btrcpy --strict-imports btrc/nixosctl/NixosCtl.btrc -o nixosctl.c
    $CC -std=c11 -O2 nixosctl.c -lm -lpthread -lutil -o nixosctl
  '';
  installPhase = ''
    mkdir -p $out/bin
    cp nixosctl $out/bin/nixosctl
    wrapProgram $out/bin/nixosctl \
      ${lib.optionalString (configPath != "") "--set NIXOS_CONFIG ${lib.escapeShellArg configPath}"} \
      --prefix PATH : ${lib.makeBinPath (with pkgs; [
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
