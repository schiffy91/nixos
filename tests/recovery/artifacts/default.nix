{ system ? builtins.currentSystem }:
let
  lock = (builtins.fromJSON (builtins.readFile ../../../flake.lock)).nodes;
  pkgs = import (builtins.fetchTree { inherit (lock.nixpkgs.locked) type owner repo rev narHash; }) { inherit system; };
  inputs.btrc = builtins.getFlake "github:${lock.btrc.locked.owner}/${lock.btrc.locked.repo}/${lock.btrc.locked.rev}";
in import ../../../config/lib/btrc.nix { inherit pkgs inputs; } {
  name = "recovery-artifact-contracts";
  src = pkgs.lib.fileset.toSource {
    root = ../../..;
    fileset = pkgs.lib.fileset.unions [ ./ArtifactsTests.btrc ../../../config/system/nixos/recovery/Artifacts.btrc ];
  };
  programs.recovery-artifact-tests = "tests/recovery/artifacts/ArtifactsTests.btrc";
  nativeBuildInputs = [ pkgs.coreutils pkgs.util-linux pkgs.sbsigntool pkgs.openssl ];
  postInstall = ''
    ./recovery-artifact-tests ${pkgs.systemd}/lib/systemd/boot/efi/systemd-boot${pkgs.stdenv.hostPlatform.efiArch}.efi ${pkgs.coreutils}/bin/install
  '';
}
