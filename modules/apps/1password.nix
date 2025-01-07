{ settings, pkgs, ... }: {
  home.packages = [
    pkgs._1password-gui
    pkgs._1password-cli
  ];
  programs._1password.enable = true;
  programs._1password-gui = {
    enable = true;
    polkitPolicyOwners = [ "${settings.user.admin.username}" ];
  };
  environment.etc."1password/custom_allowed_browsers" = {
    text = "chromium";
    mode = "0755";
  };
  xdg.configFile."1Password/ssh/agent.toml".text = ''
    [[ssh-keys]]
    vault = "Private"
  '';
}