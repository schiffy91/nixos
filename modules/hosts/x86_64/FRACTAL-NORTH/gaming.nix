{ config, pkgs, lib, host, ... }:
let
  user = config.settings.user.admin.username;
  home = "/home/${user}";
  steam = import ../../../../scripts/lib/steam.nix { inherit pkgs; };
  protonTool = "GE-Proton10-34";
in {
  imports = [ ../../../apps/sunshine/system.nix ];

  ##### Steam #####
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

  system.activationScripts.steamCompatDefault = lib.stringAfter [ "users" ] ''
    if [ -d "${home}/.local/share/Steam/config" ]; then
      ${pkgs.util-linux}/bin/runuser -u ${user} -- ${steam.setCompatTool}/bin/set-steam-compat-tool \
        --steam-path "${home}/.local/share/Steam" \
        --tool-name "${protonTool}"
    fi
  '';

  ##### Moonlight #####
  services.sunshine = {
    enable = true;
    openFirewall = false;
    autoStart = true;
    capSysAdmin = true;
  };
  settings.networking.ports.tcp = [ 47984 47989 47990 48010 ];
  settings.networking.ports.udp = (lib.range 47998 48000) ++ (lib.range 8000 8010);
}
