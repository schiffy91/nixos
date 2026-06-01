{
  inputs = {
    nixpkgs = { url = "github:NixOS/nixpkgs/nixos-unstable"; };
    nixpkgs-unstable = { url = "github:nixos/nixpkgs/nixos-unstable"; };
    home-manager = { url = "github:nix-community/home-manager"; inputs.nixpkgs.follows = "nixpkgs-unstable"; };
    plasma-manager = { url = "github:nix-community/plasma-manager"; inputs.nixpkgs.follows = "nixpkgs-unstable"; inputs.home-manager.follows = "home-manager"; };
    disko = { url = "github:nix-community/disko"; inputs.nixpkgs.follows = "nixpkgs-unstable"; };
    lanzaboote = { url = "github:nix-community/lanzaboote"; inputs.nixpkgs.follows = "nixpkgs-unstable"; };
    btrc = { url = "github:schiffy91/btrc"; inputs.nixpkgs.follows = "nixpkgs-unstable"; };
  };
  outputs = inputs@{ self, ... }:
    let
      lib = inputs.nixpkgs.lib;
      systems = [ "aarch64-darwin" "x86_64-darwin" "x86_64-linux" "aarch64-linux" ];
      pkgsFor = system: import inputs.nixpkgs { inherit system; config.allowUnfree = true; };
      eachSystem = fn: lib.genAttrs systems (system: fn system (pkgsFor system));
      isHostEntry = path: lib.hasSuffix ".nix" path && (lib.removeSuffix ".nix" (baseNameOf path)) == (baseNameOf (dirOf path));
      systemModuleFiles = lib.filter
        (path: lib.hasSuffix ".nix" path && baseNameOf path != "disk-operation.nix")
        (lib.filesystem.listFilesRecursive ./nix/system);
      mkNixosSystem = hostFile: target:
        let
          name = lib.removeSuffix ".nix" (baseNameOf hostFile);
          system = "${baseNameOf (dirOf (dirOf hostFile))}-linux";
          targetModules =
            if lib.hasInfix "Boot" target then
              [{ settings.boot.method = lib.mkForce target; }] ++ systemModuleFiles
            else
              [ ./nix/system/disk-operation.nix ];
          pkgs-unstable = import inputs.nixpkgs-unstable { inherit system; config.allowUnfree = true; };
        in {
          name = "${name}-${target}";
          value = lib.nixosSystem {
            inherit system;
            specialArgs = { inherit self inputs pkgs-unstable; };
            modules = [{
              imports = [ ./nix/settings.nix hostFile ] ++ targetModules;
              config = {
                nix = {
                  channel.enable = false;
                  settings.experimental-features = [ "nix-command" "flakes" ];
                };
                nixpkgs.config.allowUnfree = true;
                system.stateVersion = "24.11";
              };
            }];
          };
        };
    in {
      devShells = eachSystem (system: pkgs: {
        default = pkgs.mkShell {
          packages = with pkgs; [
            inputs.btrc.packages.${system}.btrcpy
            gnumake
            nixd
            git
            coreutils
            stdenv.cc
          ];
        };
      });
      nixosConfigurations = lib.listToAttrs (
        lib.concatMap (hostFile: [
          (mkNixosSystem hostFile "Disk-Operation")
          (mkNixosSystem hostFile "Standard-Boot")
          (mkNixosSystem hostFile "Secure-Boot")
        ]) (lib.filter isHostEntry (lib.filesystem.listFilesRecursive ./nix/hosts))
      );
    };
}
