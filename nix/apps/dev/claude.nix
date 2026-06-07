{ config, lib, ... }:
let
  mkHomeFiles = user:
    lib.mkIf user.homeManager.enable {
      home-manager.users.${user.username} = { config, ... }: {
        home.file.".claude/skills/nixos-init".source =
          config.lib.file.mkOutOfStoreSymlink "${config.home.homeDirectory}/.agents/skills/nixos-init";
      };
    };
in
{
  config = lib.mkIf (config.settings.apps.enable && config.settings.apps.dev.enable && config.settings.apps.claude.enable) (lib.mkMerge [
    (mkHomeFiles config.settings.users.admin)
    (lib.mkIf config.settings.users.agent.enable (mkHomeFiles config.settings.users.agent))
  ]);
}
