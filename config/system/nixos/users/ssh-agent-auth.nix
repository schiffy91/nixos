{ ... }: {
  # A stale agent socket must leave sudo privileged for password fallback.
  nixpkgs.overlays = [ (_final: prev: {
    pam_ssh_agent_auth = prev.pam_ssh_agent_auth.overrideAttrs (old: {
      patches = (old.patches or []) ++ [ ./pam-ssh-agent-auth-restore-euid.patch ];
    });
  }) ];
}
