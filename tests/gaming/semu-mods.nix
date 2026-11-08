{ inputs, pkgs }:
inputs.semu.checks.${pkgs.stdenv.hostPlatform.system}.contracts.overrideAttrs (old: {
  # The contracts also inspect PLAN.md and integration/visual fixtures omitted
  # from upstream's filtered check source.
  src = inputs.semu.outPath;
  patches = (old.patches or []) ++ [
    ../../config/pkgs/gaming/semu/portable-mods.patch
    ../../config/pkgs/gaming/semu/portable-mods-tests.patch
  ];
})
