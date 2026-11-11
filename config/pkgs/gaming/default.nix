{ config, lib, ... }: {
  imports = [
    ./assettocorsa
    ./battlenet
    ./btrsmith
    ./chiaki.nix
    ./deckpad
    ./game-run
    ./moonlight.nix
    ./proton
    ./rocksmith
    ./semu
    ./simracing
    ./steam
    ./sunshine
    ./wine
    ./wow
  ];
  settings.disk.immutability.persist.paths = lib.mkIf (config.settings.apps.gaming.enable) [
    "${config.settings.users.admin.homeDirectory}/Games"
    "${config.settings.users.admin.homeDirectory}/My Games"
    "${config.settings.users.admin.homeDirectory}/.local/share/Larian Studios"
  ];
}
