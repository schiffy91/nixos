{ pkgs, lib, ... }:
let
  edid = pkgs.callPackage ../../../apps/pkg-overrides/sunshine/edid { };
  connector = edid.passthru.connector;
  mode = "1280x800@90";
  position = "7016,0";  # just right of DP-1 (Pro Display XDR is 6016 wide); apps don't accidentally spawn here
  kd = "${pkgs.kdePackages.libkscreen}/bin/kscreen-doctor";
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
    "drm.edid_firmware=${connector}:${edid.passthru.firmwarePath}"
    "video=${connector}:d"  # disable streaming connector during boot so Plymouth uses DP-1
  ];
  hardware.firmware = [ edid ];

  # DP-3 stays always-on as a standalone display positioned off to the right.
  # No prep/undo dance — Sunshine just captures DP-3 whenever it streams.
  systemd.user.services.streaming-display-setup = {
    description = "Configure DP-3 streaming display at session start";
    wantedBy = [ "graphical-session.target" ];
    partOf = [ "graphical-session.target" ];
    after = [ "graphical-session.target" "plasma-kwin_wayland.service" ];
    serviceConfig = {
      Type = "oneshot";
      ExecStartPre = "${pkgs.coreutils}/bin/sleep 3";  # let KWin settle before we touch outputs
      ExecStart = pkgs.writeShellScript "streaming-display-setup" ''
        ${kd} output.${connector}.enable \
              output.${connector}.scale.1 \
              output.${connector}.position.${position} \
              output.${connector}.mode.${mode} \
              output.${connector}.hdr.enable || true
      '';
      RemainAfterExit = true;
    };
  };
}
