{ pkgs, inputs }:
pkgs.symlinkJoin {
  name = "gaming-tools";
  paths = [
    (import ./steam/package.nix { inherit pkgs inputs; })
    (import ./wine/package.nix { inherit pkgs inputs; })
    (import ./assettocorsa/tools.nix { inherit pkgs inputs; })
  ];
}
