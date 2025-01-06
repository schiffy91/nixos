{ config, inputs, pkgs, ... }: {
  nixpkgs.config.allowUnfree = true;
  nixpkgs-unstable.config.allowUnfree = true;
  environment.systemPackages = (with pkgs; [
    micro
    wget
    devbox
    git
    python313
    _1password-gui
    _1password-cli
    home-manager
    inputs.sbctl-pkg.legacyPackages.${pkgs.system}.sbctl
    inputs.nixpkgs-unstable.legacyPackages.${pkgs.system}.vscode
    inputs.nixpkgs-unstable.legacyPackages.${pkgs.system}.vscode.fhs
  ]);
  programs._1password.enable = true;
  programs._1password-gui = {
    enable = true;
    polkitPolicyOwners = [ "${config.variables.user.admin.username}" ];
  };
  environment.etc."1password/custom_allowed_browsers" = {
    text = "chromium";
    mode = "0755";
  };
  
  programs.nix-ld.enable = true; # See https://nixos.wiki/wiki/Visual_Studio_Code#Remote_SSH
}