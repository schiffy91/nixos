{ pkgs, ... }: {
  nixpkgs.config.allowUnfree = true;
  environment.systemPackages = with pkgs; [
    micro
    wget
    devbox
    git
    vscode
  ];
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
}