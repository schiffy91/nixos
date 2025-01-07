{ settings, pkgs, ... }: {
  # Configure 1Password SSH Agent to add all my SSH Keys
  xdg.configFile."1Password/ssh/agent.toml".text = ''
    [[ssh-keys]]
    vault = "Private"
  '';
}