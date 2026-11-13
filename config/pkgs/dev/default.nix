{ config, lib, pkgs, ... }:
{
  imports = [ ./git.nix ./zsh.nix ./vscode.nix ./antigravity.nix ./lmstudio ./distrobox.nix ./agents/persistence.nix ];
  environment.systemPackages =
    lib.optional config.settings.apps.dev.nixd.enable (import ./nixd/package.nix { inherit pkgs; })
    ++ lib.optional config.settings.apps.dev.claude.enable pkgs.claude-code
    ++ lib.optional config.settings.apps.dev.codex.enable (import ./codex/package.nix { inherit pkgs; });
}
