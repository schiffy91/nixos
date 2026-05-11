{ pkgs, host, ... }: {
  ##### Steam #####
  programs.steam = {
    enable = true;
    package = pkgs.steam.override {
      extraLibraries = pkgs': with pkgs'; [ pipewire.jack ];
      extraEnv = host.nvidiaOffloadEnv;  # Steam doesn't inherit env vars
    };
  };
}
