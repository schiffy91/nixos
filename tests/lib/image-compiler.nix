{ pkgs, inputs }:
(import ../../config/pkgs/dev/btrc/compat.nix {
  inherit pkgs;
  packages = inputs.btrc.packages.${pkgs.stdenv.buildPlatform.system};
}).btrcpy
