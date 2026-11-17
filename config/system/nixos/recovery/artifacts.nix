{ pkgs, inputs }:
inputs.btrc.lib.buildPackage pkgs {
  name = "nixos-recovery-artifacts";
  src = pkgs.lib.fileset.toSource {
    root = ./.;
    fileset = pkgs.lib.fileset.unions [ ./Artifacts.btrc ./InstallArtifacts.btrc ./Mount.btrc ./signals.h ./btrc.toml ./btrc.lock ];
  };
  programs.nixos-recovery-artifacts = "InstallArtifacts.btrc";
  runtimeInputs = [ pkgs.coreutils pkgs.util-linux pkgs.sbsigntool ];
}
