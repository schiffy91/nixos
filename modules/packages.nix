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
    efibootmgr
    _1password-gui
    _1password-cli
    alacritty
    #inputs.nixpkgs-unstable.legacyPackages.${pkgs.system}.ghostty Too buggy :(
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
    konsole
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