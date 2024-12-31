{ config, inputs, pkgs, ... }: {
  config = {
    nixpkgs.config.allowUnfree = true;
    environment.systemPackages = with pkgs; [
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
      alacritty
      inputs.ghostty.packages.${inputs.nixpkgs.system}.default
    ];
    programs._1password.enable = true;
    programs._1password-gui = {
      enable = true;
      polkitPolicyOwners = [ "${config.userConfig.rootUser}" ];
    };
    environment.etc."1password/custom_allowed_browsers" = {
      text = "chromium";
      mode = "0755";
    };
    environment.plasma6.excludePackages = with pkgs.kdePackages; [ 
      kate
      gwenview
      khelpcenter
      elisa
      ark
      okular
      print-manager
      drkonqi
      spectacle
    ];
    services.printing.browsed.enable = false;
  };
}
