{ pkgs, inputs }:
let
  tools = (import ../../lib/btrc.nix { inherit pkgs inputs; }) {
    name = "dev-shell-tools";
    src = ./shell;
    programs.dev-shell-tools = "Main.btrc";
  };
in {
  aliases = {
    clauded = "claude --dangerously-skip-permissions";
    print-files = "${tools}/bin/dev-shell-tools print-files";
    extract-substring = "${tools}/bin/dev-shell-tools extract-substring";
  };
  init = ''
    nix-shell-with-pkgs() {
      nix-shell -I nixpkgs=channel:nixos-unstable -p "$@"
    }
  '';
}
