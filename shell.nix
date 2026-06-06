{ system ? builtins.currentSystem }:
let
  flake = builtins.getFlake (toString ./.);
  pkgs = flake.inputs.nixpkgs.legacyPackages.${system};
in
pkgs.mkShell {
  packages = [
    flake.inputs.btrc.packages.${system}.btrcpy
    flake.inputs.btrc.packages.${system}.btrc-lsp
    pkgs.gnumake
    pkgs.nixd
    pkgs.git
    pkgs.coreutils
    pkgs.stdenv.cc
  ];
}
