{ inputs, pkgs, ... }: {
  environment.systemPackages = (with pkgs; [
    micro
    git
    python313
    inputs.sbctl-pkg.legacyPackages.${pkgs.system}.sbctl
  ]);
  programs.nix-ld.enable = true; # See https://nixos.wiki/wiki/Visual_Studio_Code#Remote_SSH 
}