{ config, inputs, pkgs, ... }: {
  hardware.graphics.enable = true;
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
    inputs.nixpkgs-unstable.legacyPackages.${pkgs.system}.ghostty
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
  environment.plasma6.excludePackages = (with pkgs.kdePackages; [
    kate
    gwenview
    khelpcenter
    elisa
    ark
    okular
    print-manager
    drkonqi
    spectacle
  ]);
  services.printing.browsed.enable = false;
}