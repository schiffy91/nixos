{ config, inputs, pkgs, unstable-pkgs, ... }: {
  environment.systemPackages = (with pkgs; [
    micro
    git
    python313
    inputs.sbctl-pkg.legacyPackages.${pkgs.system}.sbctl
  ]);
}