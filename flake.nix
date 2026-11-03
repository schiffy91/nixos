{
  inputs = {
    nixpkgs-darwin = { url = "github:NixOS/nixpkgs/nixpkgs-unstable"; };
    home-manager-darwin = { url = "github:nix-community/home-manager"; inputs.nixpkgs.follows = "nixpkgs-darwin"; };
    nix-darwin = { url = "github:nix-darwin/nix-darwin/master"; inputs.nixpkgs.follows = "nixpkgs-darwin"; };
    nixpkgs = { url = "github:NixOS/nixpkgs/nixos-unstable"; };
    nixpkgs-unstable = { url = "github:nixos/nixpkgs/nixos-unstable"; };
    home-manager = { url = "github:nix-community/home-manager"; inputs.nixpkgs.follows = "nixpkgs-unstable"; };
    plasma-manager = { url = "github:nix-community/plasma-manager"; inputs.nixpkgs.follows = "nixpkgs-unstable"; inputs.home-manager.follows = "home-manager"; };
    disko = { url = "github:nix-community/disko"; inputs.nixpkgs.follows = "nixpkgs-unstable"; };
    lanzaboote = { url = "github:nix-community/lanzaboote"; inputs.nixpkgs.follows = "nixpkgs-unstable"; };
    jovian = { url = "github:Jovian-Experiments/Jovian-NixOS"; inputs.nixpkgs.follows = "nixpkgs"; };
    btrc = { url = "github:schiffy91/btrc"; inputs.nixpkgs.follows = "nixpkgs-unstable"; };
    semu = { url = "github:schiffy91/semu"; inputs.nixpkgs.follows = "nixpkgs-unstable"; inputs.btrc.follows = "btrc"; };
    logitech-trueforce = { url = "github:mescon/logitech-trueforce-linux-driver"; inputs.nixpkgs.follows = "nixpkgs"; };
  };
  outputs = inputs@{ self, ... }: import ./nix/outputs.nix { inherit inputs self; };
}
