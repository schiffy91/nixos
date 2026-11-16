{ pkgs, inputs }:
(import ../lib/build-tools.nix { inherit pkgs inputs; }) {
  name = "steam-tools";
  sources = [ ./src ../lib/ModTree.btrc ];
  programs = {
    configure-steam-apps = "steam/src/Configure.btrc";
    install-steam-shortcuts = "steam/src/InstallShortcuts.btrc";
  };
}
