{ pkgs ? import (builtins.getFlake "git+file:///etc/nixos").inputs.nixpkgs {
    system = builtins.currentSystem;
    config.allowUnfree = true;
  }
}:
let
  compiler = (builtins.fromJSON (builtins.readFile ../../../../../flake.lock)).nodes.btrc.locked;
  inputs.btrc = builtins.getFlake (builtins.flakeRefToString (builtins.removeAttrs compiler [
    "narHash" "lastModified" "lastModifiedDate" "revCount" "dirtyRev" "dirtyShortRev"
  ]));
in (inputs.btrc.lib.buildPackage pkgs) {
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
