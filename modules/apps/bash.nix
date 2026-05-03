{ ... }: {
  programs.bash = {
    enable = true;
    bashrcExtra = ''
      nix-shell-unstable() {
        nix-shell -I nixpkgs=channel:nixos-unstable -p "$@"
      }
    '';
  };
}
