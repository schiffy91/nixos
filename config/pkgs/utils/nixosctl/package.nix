{ pkgs, lib, btrcpy, configPath ? "" }:
let secureBootKeys = import ./secure-boot-keys/package.nix { inherit pkgs btrcpy; };
in pkgs.stdenv.mkDerivation {
  name = "nixosctl";
  src = lib.fileset.toSource {
    root = ../../../..;
    fileset = lib.fileset.unions [ ./Main.btrc ./lib ./completions ./btrc.toml ./btrc.lock ];
  };
  nativeBuildInputs = [ btrcpy pkgs.makeWrapper ];
  dontConfigure = true;
  buildPhase = ''
    export BTRC_CACHE_DIR="$TMPDIR/btrc-cache"
    export BTRC_STATE_DIR="$TMPDIR/btrc-state"
    export BTRC_PKG_CACHE="$TMPDIR/btrc-pkgs"
    btrcpy --strict-imports config/pkgs/utils/nixosctl/Main.btrc -o nixosctl.c
    $CC -std=c11 -O2 nixosctl.c -lm -lpthread -lutil -o nixosctl
    btrcpy --strict-imports config/pkgs/utils/nixosctl/lib/PasswordHashFileMain.btrc -o password-file.c
    $CC -std=c11 -O2 password-file.c -lm -lpthread -lutil -o nixosctl-password-file
  '';
  installPhase = ''
    mkdir -p $out/bin
    cp nixosctl $out/bin/nixosctl
    cp nixosctl-password-file $out/bin/nixosctl-password-file
    ln -s ${secureBootKeys}/bin/nixosctl-secure-boot-keys $out/bin/nixosctl-secure-boot-keys
    install -Dm644 config/pkgs/utils/nixosctl/completions/_nixosctl $out/share/zsh/site-functions/_nixosctl
    install -Dm644 config/pkgs/utils/nixosctl/completions/nixosctl.bash $out/share/bash-completion/completions/nixosctl
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
        binutils
        cryptsetup
        util-linux
        coreutils
        git
        mkpasswd
        nix
      ])}
  '';
}
