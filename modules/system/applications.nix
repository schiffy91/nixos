{ config, inputs, pkgs, unstable-pkgs, ... }: {
  environment.systemPackages = (with pkgs; [
    micro
    wget
    devbox
    git
    python313
    _1password-gui
    _1password-cli
    inputs.sbctl-pkg.legacyPackages.${pkgs.system}.sbctl
    unstable-pkgs.vscode
    unstable-pkgs.vscode.fhs
  ]);
  programs.nix-ld.enable = true; # See https://nixos.wiki/wiki/Visual_Studio_Code#Remote_SSH
  programs._1password.enable = true;
  programs._1password-gui = {
    enable = true;
    polkitPolicyOwners = [ "${config.settings.user.admin.username}" ];
  };
  environment.etc."1password/custom_allowed_browsers" = {
    text = "chromium";
    mode = "0755";
  };  
}