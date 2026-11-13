{ inputs ? (builtins.getFlake "git+file://${toString ../..}").inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; config.allowUnfree = true; };
in pkgs.linkFarm "gaming-tests" [
  { name = "capture-evidence"; path = import ./capture.nix { inherit pkgs inputs; }; }
  { name = "artwork"; path = import ./artwork.nix { inherit pkgs inputs; }; }
  { name = "wow-lock"; path = import ./wow { inherit pkgs inputs; }; }
  { name = "proton-tools"; path = import ./proton/tools { inherit pkgs inputs; }; }
  { name = "runtime"; path = import ./runtime { inherit pkgs inputs; }; }
  { name = "package-boundaries"; path = import ./package-boundaries.nix { inherit pkgs inputs; }; }
  { name = "semu-contracts"; path = inputs.semu.checks.${system}.contracts; }
  { name = "sunshine-session"; path = import ./sunshine-session.nix { inherit pkgs; }; }
  { name = "proton-session"; path = import ./proton-session.nix { inherit pkgs; }; }
  { name = "affinity"; path = import ./affinity.nix { inherit pkgs inputs; }; }
  { name = "deckpad"; path = import ./deckpad { inherit pkgs inputs; }; }
  { name = "tools"; path = import ./tools.nix { inherit pkgs inputs; }; }
  { name = "wheel-menu"; path = import ../../config/pkgs/gaming/assettocorsa/wheel-menu/package.nix { inherit pkgs; }; }
]
