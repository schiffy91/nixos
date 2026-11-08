{ pkgs, ... }:
let
  printFiles = pkgs.writeShellApplication {
    name = "print-files";
    runtimeInputs = [ pkgs.coreutils pkgs.findutils ];
    text = builtins.readFile ./shell/print-files.sh;
  };
  extractSubstring = pkgs.writeShellApplication {
    name = "extract-substring";
    text = builtins.readFile ./shell/extract-substring.sh;
  };
in {
  aliases = {
    clauded = "claude --dangerously-skip-permissions";
    print-files = "${printFiles}/bin/print-files";
    extract-substring = "${extractSubstring}/bin/extract-substring";
  };
  init = ''
    nix-shell-with-pkgs() {
      nix-shell -I nixpkgs=channel:nixos-unstable -p "$@"
    }
  '';
}
