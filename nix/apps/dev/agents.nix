{ config, lib, ... }:
let
  nixosInitSkill = ''
    ---
    name: nixos-init
    description: Load full context for the NixOS configuration at /etc/nixos. Use before answering questions about this repo or changing system, disk, boot, recovery, BTRC, or simulator behavior.
    ---

    # nixos-init

    - Treat /etc/nixos as the deployed copy of this repository.
    - Read flake.nix, nix/settings.nix, nix/system, nix/hosts, btrc, and tests before changing behavior.
    - Prefer existing module boundaries and settings patterns over new abstractions.
    - For BTRC, use stdlib helpers, f-strings, structured parsers, and direct shell helpers where they make code simpler.
    - Do not reintroduce live immutability baseline recapture during nixosctl update. Immutability recovery belongs in boot/initrd paths and simulator coverage.
    - Verify disk, boot, recovery, and immutability changes with targeted Nix evals and simulator tests for x86_64 and aarch64 when feasible.

    Read the repository file list first:

    `find /etc/nixos -type f -not -path '*/.git/*' -not -path '*/.direnv/*' -not -path '*/__pycache__/*' -not -path '*/secrets/*' -not -name '*.pyc' -not -name '*.dll' -not -name '*.dll.so' -not -name '*.exe' -not -name '*.so' -not -name 'flake.lock' -not -name 'package-lock.json' | sort`
  '';

  mkHomeFiles = user:
    lib.mkIf user.homeManager.enable {
      home-manager.users.${user.username}.home.file.".agents/skills/nixos-init/SKILL.md".text = nixosInitSkill;
    };
in
{
  config = lib.mkIf (config.settings.apps.enable && config.settings.apps.dev.enable && config.settings.apps.agents.enable) (lib.mkMerge [
    (mkHomeFiles config.settings.users.admin)
    (lib.mkIf config.settings.users.agent.enable (mkHomeFiles config.settings.users.agent))
  ]);
}
