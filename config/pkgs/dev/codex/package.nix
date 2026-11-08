{ pkgs }:
pkgs.codex.overrideAttrs (_: {
  # Codex declares bundled SQLite and a prefixed jemalloc in Cargo.lock.
  # The new nixpkgs hook forces undeclared system replacements for both.
  dontCargoUseSystemLibs = true;
})
