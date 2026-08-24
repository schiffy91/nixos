{ pkgs, config, lib, inputs, ... }:
let
  btrcpy = inputs.btrc.packages.${pkgs.stdenv.hostPlatform.system}.btrcpy;
  nixosctlBin = pkgs.stdenv.mkDerivation {
    name = "nixosctl";
    src = ../../../..;
    nativeBuildInputs = [ btrcpy pkgs.makeWrapper ];
    dontConfigure = true;
    buildPhase = ''
      btrcpy --strict-imports btrc/nixosctl/nixosctl.btrc -o nixosctl.c
      $CC -std=c11 -O2 nixosctl.c -lm -lpthread -lutil -o nixosctl
    '';
    installPhase = ''
      mkdir -p $out/bin
      cp nixosctl $out/bin/nixosctl
      wrapProgram $out/bin/nixosctl \
        ${lib.optionalString (config.settings.nixosctl.configPath != "")
          "--set NIXOS_CONFIG ${lib.escapeShellArg config.settings.nixosctl.configPath}"} \
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
          perl
          nix
        ])}
    '';
  };
in lib.mkIf (config.settings.apps.enable && config.settings.apps.utils.enable && config.settings.apps.nixosctl.enable) {
  environment.systemPackages = [ nixosctlBin ];
}
