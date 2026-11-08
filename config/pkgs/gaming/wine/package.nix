{ pkgs, inputs }:
(import ../lib/build-tools.nix { inherit pkgs inputs; }) {
  name = "wine-prefix-dpi";
  sources = [ ./src ../lib/GamingFiles.btrc ];
  programs.wine-prefix-dpi = "wine/src/Dpi.btrc";
}
