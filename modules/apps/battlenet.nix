{ config, pkgs, lib, ... }:
let
  steam = import ../lib/steam.nix { inherit pkgs; };
  launchOptions =
    "PROTON_ENABLE_WAYLAND=1 PROTON_ENABLE_HDR=1 DXVK_HDR=1 %command%";
  steamPath = "${config.home.homeDirectory}/.local/share/Steam";
in {
  home.activation.battlenet = lib.hm.dag.entryAfter [ "writeBoundary" ] ''
    if [ -d "${steamPath}/userdata" ]; then
      ${steam.setLaunchOptions}/bin/set-steam-launch-options \
        --steam-path "${steamPath}" \
        --launch-options "${launchOptions}" \
        --name-match "battle.net"
    fi
  '';
}
