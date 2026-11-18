{ config, ... }: {
  security.rtkit.enable = true;
  services.pulseaudio.enable = false;
  services.pipewire = {
    enable = true;
    alsa.enable = true;
    pulse.enable = true;  # JACK clients use pw-jack; Steam carries pipewire.jack itself
  };
  settings.disk.immutability.persist.paths = [
    "${config.settings.users.admin.homeDirectory}/.local/state/wireplumber"  # each device's chosen profile, routes and volumes
  ];
}
