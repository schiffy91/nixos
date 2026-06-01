{ pkgs, config, lib, buildBtrcProgram, ... }:
let
  nixosctlBin = buildBtrcProgram {
    name = "nixosctl";
    entry = "btrc/system/nixosctl.btrc";
    nativeBuildInputs = [ pkgs.makeWrapper ];
    extraLibs = [ "-lm" "-lpthread" "-lutil" ];
    extraInstall = ''
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
