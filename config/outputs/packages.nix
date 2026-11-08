{ inputs, installerPackages }:
let
  system = "x86_64-linux";
  pkgs = import inputs.nixpkgs { inherit system; config.allowUnfree = true; };
in installerPackages // {
  ${system} = installerPackages.${system} // {
    slopsmith = import ../pkgs/gaming/slopsmith/package.nix { inherit pkgs inputs; };
  };
}
