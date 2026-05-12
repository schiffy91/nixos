{ config, pkgs, lib, ... }: let
  user = config.settings.user.admin.username;
  agentToml = pkgs.writeText "1password-ssh-agent.toml" ''
    [[ssh-keys]]
    vault = "Private"
  '';
in {
  environment.systemPackages = with pkgs; [ _1password-gui _1password-cli ];
  system.activationScripts.onePasswordAgent = lib.stringAfter [ "users" ] ''
    ${pkgs.coreutils}/bin/install -d -o ${user} -m 0700 /home/${user}/.config/1Password/ssh
    ${pkgs.coreutils}/bin/install -o ${user} -m 0600 ${agentToml} /home/${user}/.config/1Password/ssh/agent.toml
  '';
}
