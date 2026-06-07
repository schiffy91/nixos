{
  description = "Slopsmith configured for the local Rocksmith DLC library";

  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs {
        inherit system;
        config.allowUnfree = true;
      };
      slopsmith = pkgs.callPackage ./package.nix { };
    in {
      packages.${system} = {
        default = slopsmith;
        slopsmith = slopsmith;
      };

      apps.${system} = {
        default = {
          type = "app";
          program = "${slopsmith}/bin/slopsmith";
        };
        slopsmith = self.apps.${system}.default;
      };
    };
}
