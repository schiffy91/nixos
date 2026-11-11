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
    # deckpad needs the stdlib's USB, HID, Bluetooth, Network and Crypto groups, which are on this
    # branch until it is merged; then this goes back to "github:schiffy91/btrc".
    btrc = { url = "github:schiffy91/btrc/claude/dazzling-einstein-fg9e06"; inputs.nixpkgs.follows = "nixpkgs-unstable"; };
    semu = { url = "github:schiffy91/semu"; inputs.nixpkgs.follows = "nixpkgs-unstable"; inputs.btrc.follows = "btrc"; };
    logitech-trueforce = { url = "github:mescon/logitech-trueforce-linux-driver"; inputs.nixpkgs.follows = "nixpkgs"; };
  };
  outputs = inputs@{ self, ... }: import ./config/outputs { inherit inputs self; };
}
