{ config, ... }: let
  user = config.settings.user.admin.username;
in {
  home-manager.users.${user}.home.file.".claude/skills/nixos-init/SKILL.md".text = ''
    ---
    name: nixos-init
    description: Load full context for the NixOS configuration at /etc/nixos by reading the flake and every module file. Use when starting work on this repo, before answering questions about the system config, or when the user asks to init/load the nixos config.
    ---

    # nixos-init

    1) Read every code file in /etc/nixos. Run this command to get the full list, then read each file:

    `find /etc/nixos -type f -not -path '*/.git/*' -not -path '*/.direnv/*' -not -path '*/__pycache__/*' -not -path '*/secrets/*' -not -name '*.pyc' -not -name '*.dll' -not -name '*.dll.so' -not -name '*.exe' -not -name '*.so' -not -name 'flake.lock' -not -name 'package-lock.json' | sort`

    2) Make a git worktree with a short and succinct label for the project.
    3) If existing changes in git, ask me what to do with them.
  '';
}
