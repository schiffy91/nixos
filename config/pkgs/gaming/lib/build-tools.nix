{ pkgs, inputs }:
{ name, sources, programs, buildInputs ? [], runtimeInputs ? [ pkgs.coreutils pkgs.procps ], linkFlags ? [] }:
(import ../../../lib/btrc.nix { inherit pkgs inputs; }) {
  inherit name buildInputs runtimeInputs linkFlags;
  programs = pkgs.lib.mapAttrs (_: entry: "pkgs/gaming/${entry}") programs;
  src = pkgs.lib.fileset.toSource {
    root = ../../..;
    fileset = pkgs.lib.fileset.unions sources;
  };
}
