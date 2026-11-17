{ system ? builtins.currentSystem }:
let
  lock = (builtins.fromJSON (builtins.readFile ../../../flake.lock)).nodes;
  pkgs = import (builtins.fetchTree { inherit (lock.nixpkgs.locked) type owner repo rev narHash; }) { inherit system; };
  inputs.btrc = builtins.getFlake (builtins.flakeRefToString (builtins.removeAttrs lock.btrc.locked [ "narHash" "lastModified" "lastModifiedDate" "revCount" "dirtyRev" "dirtyShortRev" ]));
in inputs.btrc.lib.buildPackage pkgs {
  name = "recovery-artifact-contracts";
  src = pkgs.lib.fileset.toSource {
    root = ../../..;
    fileset = pkgs.lib.fileset.unions [ ./ArtifactsTests.btrc ./btrc.toml ./btrc.lock ../../../config/system/nixos/recovery/Artifacts.btrc ../../../config/system/nixos/recovery/Mount.btrc ../../../config/system/nixos/recovery/signals.h ../../../config/system/nixos/recovery/btrc.toml ../../../config/system/nixos/recovery/btrc.lock ];
  };
  programs.recovery-artifact-tests = "tests/recovery/artifacts/ArtifactsTests.btrc";
  nativeBuildInputs = [ pkgs.coreutils pkgs.util-linux pkgs.sbsigntool pkgs.openssl ];
  postInstall = ''
    ./recovery-artifact-tests ${pkgs.systemd}/lib/systemd/boot/efi/systemd-boot${pkgs.stdenv.hostPlatform.efiArch}.efi ${pkgs.coreutils}/bin/install
  '';
}
