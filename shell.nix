{ system ? builtins.currentSystem }:
let
  flake = builtins.getFlake "git+file://${toString ./.}";
  pkgs = flake.inputs.nixpkgs.legacyPackages.${system};
in
pkgs.mkShell {
  packages = [
    flake.inputs.btrc.packages.${system}.btrcpy
    pkgs.gnumake
    pkgs.nixd
    pkgs.git
    pkgs.coreutils
    pkgs.stdenv.cc
  ];
}
