{ pkgs, lib, host, ... }:
let
  edid      = pkgs.callPackage ../../../apps/pkg-overrides/sunshine/edid { };
  kd        = "${pkgs.kdePackages.libkscreen}/bin/kscreen-doctor";
  streaming = host.display.streaming;
in {
  services.sunshine = {
    enable = true;
    openFirewall = false;
    autoStart = true;
    capSysAdmin = true;
  };
  settings.networking.ports.tcp = [ 47984 47989 47990 48010 ];
  settings.networking.ports.udp = (lib.range 47998 48000) ++ (lib.range 8000 8010);

  ##### Streaming Display #####
  boot.kernelParams = [
    "drm.edid_firmware=${streaming.connector}:${edid.passthru.firmwarePath}"
    "video=${streaming.connector}:d"  # disable streaming connector during boot so Plymouth uses DP-1
  ];
  hardware.firmware = [ edid ];

  # DP-3 stays always-on as a standalone display positioned off to the right.
  # No prep/undo dance — Sunshine just captures DP-3 whenever it streams.
  systemd.user.services.streaming-display-setup = {
    description = "Configure ${streaming.connector} streaming display at session start";
    wantedBy = [ "graphical-session.target" ];
    partOf = [ "graphical-session.target" ];
    after = [ "graphical-session.target" "plasma-kwin_wayland.service" ];
    serviceConfig = {
      Type = "oneshot";
      ExecStartPre = "${pkgs.coreutils}/bin/sleep 3";  # let KWin settle before we touch outputs
      ExecStart = pkgs.writeShellScript "streaming-display-setup" ''
        ${kd} output.${streaming.connector}.enable \
              output.${streaming.connector}.scale.1 \
              output.${streaming.connector}.position.${streaming.position} \
              output.${streaming.connector}.mode.${streaming.mode} \
              output.${streaming.connector}.hdr.enable || true
      '';
      RemainAfterExit = true;
    };
  };
}
