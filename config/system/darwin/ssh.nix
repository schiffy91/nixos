{ config, lib, ... }:
let
  admin = config.settings.users.admin;
  networking = config.settings.networking;
  agentSocket = "Library/Group Containers/2BUA8C4S2C.com.1password/t/agent.sock"; # fixed by 1Password for macOS
in {
  settings.networking.identityAgent = lib.mkDefault "~/${agentSocket}";
  services.openssh = {
    enable = config.settings.macos.remoteLogin.enable;
    extraConfig = ''
      PasswordAuthentication no
      KbdInteractiveAuthentication no
      PermitRootLogin no
    '';
  };
  users.users.${admin.username}.openssh.authorizedKeys.keys = admin.authorizedKeys;
  programs.ssh.extraConfig = import ../ssh-client.nix { inherit lib; inherit (networking) identityAgent forwardAgentHosts; };
  launchd.user.agents.onepassword-ssh-auth-sock = lib.mkIf (networking.identityAgent != "none") {
    serviceConfig = {
      ProgramArguments = [ "/bin/sh" "-c" "/bin/ln -sf \"$HOME/${agentSocket}\" \"$SSH_AUTH_SOCK\"" ]; # programs that ignore ssh_config reach 1Password too
      RunAtLoad = true;
    };
  };
}
