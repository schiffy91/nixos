{ config, lib, ... }: lib.mkIf (config.settings.apps.enable && config.settings.apps.dev.enable && config.settings.apps.bash.enable) {
  programs.bash.interactiveShellInit = ''
    nix-shell-with-pkgs() {
      nix-shell -I nixpkgs=channel:nixos-unstable -p "$@"
    }
  '';
}
