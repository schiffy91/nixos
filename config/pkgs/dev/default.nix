{ config, lib, pkgs, ... }:
let enabled = config.settings.apps.enable && config.settings.apps.dev.enable;
in {
  imports = [ ./git.nix ./zsh.nix ./vscode.nix ./lmstudio ./distrobox.nix ./agents/persistence.nix ];
  environment.systemPackages = lib.optionals (enabled && config.settings.apps.desktop.enable) (
    [ (import ./nixd/package.nix { inherit pkgs; }) ]
    ++ lib.optional config.settings.apps.claude.enable pkgs.claude-code
    ++ lib.optional config.settings.apps.codex.enable pkgs.codex
  );
  settings.apps.zsh.enable = lib.mkIf config.settings.apps.desktop.enable (lib.mkDefault true);
}
