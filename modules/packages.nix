{ config, lib, inputs, pkgs, ... }: {
  nixpkgs.config.allowUnfree = lib.mkDefault true;
  environment.systemPackages = lib.mkDefault (with pkgs; [
    micro
    wget
    devbox
    git
    vscode
    (inputs.sbctl-pkg.legacyPackages.${pkgs.system}.sbctl)
    python313
    efibootmgr
    _1password-gui
    _1password-cli
    #alacritty
    #inputs.ghostty.packages.${pkgs.system}.default
  ]);
  programs._1password.enable = lib.mkDefault true;
  programs._1password-gui = lib.mkDefault {
    enable = true;
    polkitPolicyOwners = [ "${config.userConfig.rootUser}" ];
  };
  environment.etc."1password/custom_allowed_browsers" = lib.mkDefault {
    text = "chromium";
    mode = "0755";
  };
  environment.plasma6.excludePackages = lib.mkDefault (with pkgs.kdePackages; [ 
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
  services.printing.browsed.enable = lib.mkDefault false;
}
