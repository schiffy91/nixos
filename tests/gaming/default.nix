{ inputs ? (builtins.getFlake "git+file://${toString ../..}").inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; config.allowUnfree = true; };
in pkgs.linkFarm "gaming-tests" [
  { name = "wow-lock"; path = import ./wow { inherit pkgs inputs; }; }
  { name = "proton-tools"; path = import ./proton/tools { inherit pkgs inputs; }; }
  { name = "runtime"; path = import ./runtime { inherit pkgs inputs; }; }
  { name = "package-boundaries"; path = import ./package-boundaries.nix { inherit pkgs inputs; }; }
  { name = "semu-mods"; path = import ./semu-mods.nix { inherit inputs pkgs; }; }
  { name = "sunshine-session"; path = import ./sunshine-session.nix { inherit pkgs; }; }
  { name = "proton-session"; path = import ./proton-session.nix { inherit pkgs; }; }
  { name = "affinity"; path = import ./affinity.nix { inherit pkgs; }; }
  { name = "tools"; path = import ./tools.nix { inherit pkgs inputs; }; }
  { name = "wheel-menu"; path = import ../../config/pkgs/gaming/assettocorsa/wheel-menu/package.nix { inherit pkgs; }; }
]
