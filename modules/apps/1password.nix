{ pkgs, ... }: {
  home.packages = [
    pkgs._1password-gui
    pkgs._1password-cli
  ];
  xdg.configFile."1Password/ssh/agent.toml".text = ''
    [[ssh-keys]]
    vault = "Private"
  '';
}