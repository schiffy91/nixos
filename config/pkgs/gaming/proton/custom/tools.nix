{ pkgs ? import (builtins.getFlake "git+file:///etc/nixos").inputs.nixpkgs {
    system = builtins.currentSystem;
    config.allowUnfree = true;
  }
}:
let
  compiler = (builtins.fromJSON (builtins.readFile ../../../../../flake.lock)).nodes.btrc.locked;
  inputs.btrc = builtins.getFlake "github:${compiler.owner}/${compiler.repo}/${compiler.rev}";
in (import ../../../../lib/btrc.nix { inherit pkgs inputs; }) {
  name = "proton-custom-tools";
  src = ./tools;
  programs.proton-custom-tools = "Main.btrc";
  runtimeInputs = [ pkgs.nix pkgs.git pkgs.rsync pkgs.coreutils pkgs.diffutils ];
  postInstall = ''
    for command in dev build install make; do
      makeWrapper "$out/bin/proton-custom-tools" "$out/bin/proton-custom-$command" --add-flags "$command"
    done
  '';
}
