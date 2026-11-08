{ pkgs, inputs }:
let
  steam = import ../../config/pkgs/gaming/steam/package.nix { inherit pkgs inputs; };
  wine = import ../../config/pkgs/gaming/wine/package.nix { inherit pkgs inputs; };
  assetto = import ../../config/pkgs/gaming/assettocorsa/tools.nix { inherit pkgs inputs; };
in
assert builtins.pathExists "${steam.src}/pkgs/gaming/steam/src/Configure.btrc";
assert builtins.pathExists "${wine.src}/pkgs/gaming/wine/src/Dpi.btrc";
assert !builtins.pathExists "${steam.src}/pkgs/gaming/assettocorsa";
assert !builtins.pathExists "${steam.src}/pkgs/gaming/wine";
assert !builtins.pathExists "${wine.src}/pkgs/gaming/steam";
assert !builtins.pathExists "${wine.src}/pkgs/gaming/assettocorsa";
assert builtins.pathExists "${assetto.src}/pkgs/gaming/steam/src/SteamLibrary.btrc";
assert !builtins.pathExists "${assetto.src}/pkgs/gaming/steam/src/Configure.btrc";
assert !builtins.pathExists "${assetto.src}/pkgs/gaming/wine/src/Dpi.btrc";
pkgs.runCommand "gaming-package-boundaries" {} ''touch "$out"''
