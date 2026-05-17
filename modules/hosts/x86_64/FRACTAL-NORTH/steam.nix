{ pkgs, host, ... }: {
  programs.steam = {
    enable = true;
    package = pkgs.steam.override {
      extraLibraries = pkgs': with pkgs'; [ pipewire.jack ];
      extraEnv = host.nvidiaOffloadEnv // {  # Steam doesn't inherit env vars
        PROTON_ENABLE_WAYLAND = "1";
        PROTON_ENABLE_HDR = "1";
        ENABLE_HDR_WSI = "1";
      };
    };
  };
}
