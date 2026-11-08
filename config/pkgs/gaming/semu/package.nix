{ inputs, pkgs }:
let
  packages = inputs.semu.packages.${pkgs.stdenv.hostPlatform.system};
  semuProgram = packages.semu-program.overrideAttrs (old: {
    patches = (old.patches or []) ++ [ ./portable-mods.patch ];
  });
  semuCli = packages.semu-cli.override { inherit semuProgram; };
in packages.semu.override { inherit semuCli; }
