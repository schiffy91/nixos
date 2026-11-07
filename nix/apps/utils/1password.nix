{ config, pkgs, lib, ... }: let
  user = config.settings.users.admin.username;
  agentToml = pkgs.writeText "1password-ssh-agent.toml" ''
    [[ssh-keys]]
    vault = "Private"
  '';
in lib.mkIf (config.settings.apps.enable && config.settings.apps.utils.enable && config.settings.apps.onePassword.enable) {
  programs._1password.enable = true;
  programs._1password-gui = {
    enable = true;
    polkitPolicyOwners = [ user ];
  };
  system.activationScripts.onePasswordAgent = lib.stringAfter [ "users" ] ''
    ${pkgs.coreutils}/bin/install -d -o ${user} -m 0700 ${lib.escapeShellArg "${config.settings.users.admin.homeDirectory}/.config/1Password/ssh"}
    ${pkgs.coreutils}/bin/install -o ${user} -m 0600 ${agentToml} ${lib.escapeShellArg "${config.settings.users.admin.homeDirectory}/.config/1Password/ssh/agent.toml"}
  '';
}
