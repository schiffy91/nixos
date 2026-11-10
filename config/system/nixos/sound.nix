_: {
  security.rtkit.enable = true;
  services.pulseaudio.enable = false;
  services.pipewire = {
    enable = true;
    alsa.enable = true;
    pulse.enable = true;  # JACK clients use pw-jack; Steam carries pipewire.jack itself
  };
}
