{ pkgs, config, lib, btrcStdlib, ... }:
let
  # Build nixosctl from the checked-in transpiled C (mirrors
  # semipermeableMembraneBin in modules/system/immutability.nix). Regenerate
  # generated/nixosctl.c with `make generated` after editing bin/nixosctl.btrc.
  nixosctlBin = pkgs.stdenv.mkDerivation {
    name = "nixosctl";
    dontUnpack = true;
    nativeBuildInputs = [ pkgs.makeWrapper ];
    # Program-only C linked against the precompiled stdlib archive
    # (modules/apps/btrc.nix), instead of inlining the whole stdlib.
    buildPhase = ''
      $CC -std=c11 -O2 -I${btrcStdlib.incDir} ${btrcStdlib.gcFlags} \
        ${../../generated/nixosctl.c} ${btrcStdlib.stdlibInput} \
        -lm -lpthread -lutil -o nixosctl
    '';
    installPhase = ''
      mkdir -p $out/bin
      cp nixosctl $out/bin/nixosctl
      wrapProgram $out/bin/nixosctl \
        ${lib.optionalString (config.settings.nixosHelper.configPath != "")
          "--set NIXOS_CONFIG ${lib.escapeShellArg config.settings.nixosHelper.configPath}"} \
        --prefix PATH : ${lib.makeBinPath (with pkgs; [
          kdePackages.libkscreen
          kdePackages.konsole
          systemd
          pulseaudio
          sbctl
          btrfs-progs
          cryptsetup
          util-linux
          coreutils
          git
          mkpasswd
          nix
        ])}
    '';
  };
in lib.mkIf (config.settings.apps.enable && config.settings.apps.nixosHelper.enable) {
  environment.systemPackages = [ nixosctlBin ];
}
