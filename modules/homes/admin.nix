{ variables, ... }: {
  home = {
    username = variables.user.admin;
    homeDirectory = "/home/${variables.user.admin}";
    stateVersion = "24.11";
  };
  programs.home-manager.enable = true;
  # Configure 1Password SSH Agent to add all my SSH Keys
  xdg.configFile."1Password/ssh/agent.toml".text = ''
      [[ssh-keys]]
      vault = "Private"
    '';
}