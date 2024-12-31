{ lib, ... }: {
  hardware.pulseaudio.enable = false;
  security.rtkit.enable = true;
  services.pipewire = lib.mkDefault {
    enable = true;
    alsa.enable = true;
    pulse.enable = true;
    jack.enable = true;
  };
}