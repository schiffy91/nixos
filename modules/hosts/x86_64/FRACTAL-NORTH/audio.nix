{ config, host, pkgs, ... }: {
  ##### Rocksmith / Quad Cortex #####
  users.users.${config.settings.user.admin.username}.extraGroups = [ "audio" "rtkit" "pipewire" ];
  security.pam.loginLimits = [
    { domain = "@audio"; item = "memlock"; type = "-"; value = "unlimited"; }
    { domain = "@audio"; item = "rtprio"; type = "-"; value = "99"; }
  ];
  services.pipewire = {
    extraConfig.pipewire."10-low-latency"."context.properties" = {
      "default.clock.min-quantum" = config.settings.rocksmith.sampleSize;
      "default.clock.rate" = config.settings.rocksmith.sampleRate;
      "default.clock.allowed-rates" = [ config.settings.rocksmith.sampleRate ];
    };
    wireplumber.extraConfig."51-alsa-tweaks"."monitor.alsa.rules" = [
      {
        matches = [{ "node.name" = host.audio.quadCortexInputPattern; }];
        actions.update-props = {
          "session.suspend-timeout-seconds" = 0;
          "priority.session" = 2500;
          "priority.driver" = 2500;
        };
      }
      {
        matches = [{ "node.name" = host.audio.quadCortexOutputPattern; }];
        actions.update-props = {
          "session.suspend-timeout-seconds" = 0;
          "node.driver" = false;
        };
      }
      {
        matches = [{ "node.name" = host.audio.logitechCameraInputPattern; }];
        actions.update-props."node.disabled" = true;
      }
    ];
  };
  ##### External-volume sinks always 100% #####
  # HDMI (S89C / S102) and Quad Cortex (Kanto TUK) all control their own
  # volume — wireplumber/Plasma keep restoring smaller values, so we re-pin
  # 100% on every sink appearance.
  systemd.user.services.audio-external-volume = {
    description = "Force HDMI + Quad Cortex sinks to 100% on every appearance";
    wantedBy = [ "default.target" ];
    after = [ "pipewire-pulse.service" "wireplumber.service" ];
    serviceConfig = {
      Type = "simple";
      Restart = "on-failure";
      RestartSec = 5;
    };
    path = with pkgs; [ pulseaudio coreutils gnugrep ];
    script = ''
      pin_volumes() {
        pactl list short sinks 2>/dev/null \
          | grep -iE '(hdmi|quad_cortex)' \
          | cut -f2 \
          | while read -r sink; do
              pactl set-sink-volume "$sink" 100% 2>/dev/null || true
            done
      }
      for _ in $(seq 1 30); do  # wait for pipewire-pulse to be ready
        pactl info >/dev/null 2>&1 && break
        sleep 1
      done
      pin_volumes
      pactl subscribe | while read -r line; do
        case "$line" in
          "Event 'new' on sink"*) sleep 0.5; pin_volumes ;;
        esac
      done
    '';
  };
}
