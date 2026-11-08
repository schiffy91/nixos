{ config, inputs, pkgs, lib, host, ... }:
let
  user = config.settings.users.admin.username;
  icons = import ../../../../pkgs/gaming/icons/package.nix { inherit pkgs inputs; };
  emulationRoot = "/run/media/${user}/${host.sdCard.label}/Emulation";
in {
  settings.apps = {
    gaming.packages = with pkgs; [ protonup-qt ];
    moonlight.enable = true;
    chiaki.enable = true;
    desktop.enable = true;
    nixosctl.caffeine.enable = false;
    steam = {
      protonCustom.enable = true;
      shortcuts = {
        "Battle.net" = { exe = "/etc/profiles/per-user/${user}/bin/battlenet"; icon = "${icons.battlenet}"; };
        Semu = { exe = "/run/current-system/sw/bin/semu-es-de"; icon = "${icons.semu}"; };
        Chrome = { exe = "/run/current-system/sw/bin/google-chrome-stable"; icon = icons.chrome; };
      } // lib.optionalAttrs config.settings.apps.moonlight.enable {
        Moonlight = { exe = "/run/current-system/sw/bin/moonlight"; icon = "${icons.moonlight}"; };
      } // lib.optionalAttrs config.settings.apps.chiaki.enable {
        chiaki-ng = { exe = "/run/current-system/sw/bin/chiaki"; icon = icons.chiaki; };
      };
    };
    sunshine = {
      capture = "auto";  # KWin virtual display in Plasma; KMS in Gaming Mode
    };
    battlenet.enable = true;
    rocksmith.enable = false;
    assettocorsa.enable = false;
    semu = {
      target = "steam-deck";
      root = emulationRoot;
    };
  };
}
