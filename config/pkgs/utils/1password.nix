{ config, pkgs, lib, ... }: let
  user = config.settings.users.admin.username;
  agentToml = pkgs.writeText "1password-ssh-agent.toml" ''
    [[ssh-keys]]
    vault = "Private"
  '';
  localAgent = ''
    # PAM and ssh-add use SSH_AUTH_SOCK, not ssh_config's IdentityAgent.
    # Preserve forwarded agents and the invoking user's socket under sudo.
    if [ "$(id -un)" = ${lib.escapeShellArg user} ] && [ -z "''${SSH_CONNECTION:-}" ] && [ -z "''${SUDO_USER:-}" ]; then
      export SSH_AUTH_SOCK=${lib.escapeShellArg "${config.settings.users.admin.homeDirectory}/.1password/agent.sock"}
    fi
  '';
in lib.mkIf (config.settings.apps.utils.onePassword.enable) {
  settings.disk.immutability.persist.paths = [ "${config.settings.users.admin.homeDirectory}/.config/1Password" ];
  programs._1password.enable = true;
  programs._1password-gui = {
    enable = true;
    polkitPolicyOwners = [ user ];
  };
  environment.extraInit = lib.mkBefore localAgent;
  environment.interactiveShellInit = localAgent;
  system.activationScripts.onePasswordAgent = lib.stringAfter [ "users" ] ''
    ${pkgs.coreutils}/bin/install -d -o ${user} -m 0700 ${lib.escapeShellArg "${config.settings.users.admin.homeDirectory}/.config/1Password/ssh"}
    ${pkgs.coreutils}/bin/install -o ${user} -m 0600 ${agentToml} ${lib.escapeShellArg "${config.settings.users.admin.homeDirectory}/.config/1Password/ssh/agent.toml"}
  '';
}
