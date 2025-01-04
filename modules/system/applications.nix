{ config, inputs, pkgs, ... }: {
  nixpkgs.config.allowUnfree = true;
  environment.systemPackages = (with pkgs; [
    micro
    wget
    devbox
    git
    vscode
    inputs.sbctl-pkg.legacyPackages.${pkgs.system}.sbctl
    python313
    _1password-gui
    _1password-cli
    blackbox-terminal
    home-manager
    #inputs.nixpkgs-unstable.legacyPackages.${pkgs.system}.ghostty # Too buggy in Wayland w/ VirGL; doesn't work in X11 w/ VirGL
    #alacritty # Too buggy in Wayland w/ VirGL; doesn't work in X11 w/ VirGL
  ]);
  programs._1password.enable = true;
  programs._1password-gui = {
    enable = true;
    polkitPolicyOwners = [ "${config.variables.user.admin}" ];
  };
  environment.etc."1password/custom_allowed_browsers" = {
    text = "chromium";
    mode = "0755";
  };
}