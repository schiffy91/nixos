{ config, pkgs, lib, ... }: let
  user = config.settings.users.admin.username;
  group = config.users.users.${user}.group;
  home = config.settings.users.admin.homeDirectory;
  store = "${home}/.config/1Password";  # the app keeps its own files here and cannot start without writing them
  agentToml = pkgs.writeText "1password-ssh-agent.toml" ''
    [[ssh-keys]]
    vault = "Private"
  '';
  localAgent = ''
    # PAM and ssh-add use SSH_AUTH_SOCK, not ssh_config's IdentityAgent.
    # Preserve forwarded agents and the invoking user's socket under sudo.
    if [ "$(id -un)" = ${lib.escapeShellArg user} ] && [ -z "''${SSH_CONNECTION:-}" ] && [ -z "''${SUDO_USER:-}" ]; then
      export SSH_AUTH_SOCK=${lib.escapeShellArg "${home}/.1password/agent.sock"}
    fi
  '';
in lib.mkIf (config.settings.apps.utils.onePassword.enable) {
  settings.disk.immutability.persist.paths = [ store ];
  programs._1password.enable = true;
  programs._1password-gui = {
    enable = true;
    polkitPolicyOwners = [ user ];
  };
  environment.extraInit = lib.mkBefore localAgent;
  environment.interactiveShellInit = localAgent;
  system.activationScripts.onePasswordAgent = lib.stringAfter [ "userFolder" ] ''
    user_folder ${lib.escapeShellArg "${store}/ssh"}
    ${pkgs.coreutils}/bin/chmod 0700 ${lib.escapeShellArgs [ store "${store}/ssh" ]}
    ${pkgs.coreutils}/bin/install -o ${user} -g ${group} -m 0600 ${agentToml} ${lib.escapeShellArg "${store}/ssh/agent.toml"}
  '';
}
