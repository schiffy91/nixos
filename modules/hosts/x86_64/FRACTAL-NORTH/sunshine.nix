{ lib, ... }: {
  ##### Moonlight #####
  # capSysAdmin breaks NVENC: setcap → AT_SECURE → LD_LIBRARY_PATH stripped → libcuda dlopen fails.
  services.sunshine = {
    enable = true;
    openFirewall = false;
    autoStart = true;
    capSysAdmin = false;
  };
  settings.networking.ports.tcp = [ 47984 47989 47990 48010 ];
  settings.networking.ports.udp = (lib.range 47998 48000) ++ (lib.range 8000 8010);
}
