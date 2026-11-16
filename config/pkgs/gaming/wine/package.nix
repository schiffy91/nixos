{ pkgs, inputs }:
(import ../lib/build-tools.nix { inherit pkgs inputs; }) {
  name = "wine-prefix-dpi";
  sources = [ ./src ../lib/ModTree.btrc ];
  programs.wine-prefix-dpi = "wine/src/Dpi.btrc";
  runtimeInputs = [ pkgs.coreutils pkgs.procps pkgs.kdePackages.libkscreen ];  # kscreen-doctor reports the live scale
}
