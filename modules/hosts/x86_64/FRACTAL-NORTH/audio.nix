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
  ##### HDMI sinks default to 100% (S89C / S102 control their own volume) #####
  systemd.user.services.audio-hdmi-default-volume = {
    description = "Reset HDMI audio sinks to 100% at session start";
    wantedBy = [ "default.target" ];
    after = [ "pipewire-pulse.service" "wireplumber.service" ];
    serviceConfig.Type = "oneshot";
    path = with pkgs; [ pulseaudio coreutils gnugrep ];
    script = ''
      sleep 3
      for sink in $(pactl list short sinks | grep -i hdmi | cut -f2); do
        pactl set-sink-volume "$sink" 100% || true
      done
    '';
  };
}
