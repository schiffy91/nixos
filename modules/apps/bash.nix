{ ... }: {
  programs.bash.interactiveShellInit = ''
    nix-shell-with-pkgs() {
      nix-shell -I nixpkgs=channel:nixos-unstable -p "$@"
    }
  '';
}
