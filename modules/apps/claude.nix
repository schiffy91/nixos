{ config, ... }: let
  user = config.settings.user.admin.username;
in {
  home-manager.users.${user}.home.file.".claude/skills/nixos-init/SKILL.md".text = ''
    ---
    name: nixos-init
    description: Load full context for the NixOS configuration at /etc/nixos by reading the flake and every module file. Use when starting work on this repo, before answering questions about the system config, or when the user asks to init/load the nixos config.
    ---

    # nixos-init

    Read /etc/nixos/flake.nix and every file recursively under /etc/nixos/modules and /etc/nixos/scripts.
  '';
}
