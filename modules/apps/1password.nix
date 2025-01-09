{ pkgs, ... }: {
  home.packages = with pkgs; [
    _1password-gui
    _1password-cli
  ];
  xdg.configFile."1Password/ssh/agent.toml".text = ''
    [[ssh-keys]]
    vault = "Private"
  '';
}