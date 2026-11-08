{ pkgs }:
let
  # nixf's generated table omits flake builtins supplied through extraPrimOps.
  # https://github.com/nix-community/nixd/issues/762
  nixf = pkgs.nixf.overrideAttrs (old: {
    patches = (old.patches or []) ++ [ ./flake-primops.patch ];
    buildInputs = old.buildInputs ++ [ pkgs.nixVersions.latest.libs.nix-flake ];
  });
in (pkgs.nixd.override { inherit nixf; }).overrideAttrs (old: {
  passthru = (old.passthru or {}) // { inherit nixf; };
})
