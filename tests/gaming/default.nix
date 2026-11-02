{ inputs ? (builtins.getFlake "git+file://${toString ../..}").inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; config.allowUnfree = true; };
in pkgs.linkFarm "gaming-tests" [
  { name = "tools"; path = import ../../nix/apps/gaming/package.nix { inherit pkgs inputs; }; }
  { name = "wheel-menu"; path = import ../../nix/apps/gaming/assettocorsa/wheel-menu/package.nix { inherit pkgs; }; }
]
