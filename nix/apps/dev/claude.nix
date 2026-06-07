{ config, lib, ... }:
let
  skillNames = [ "engineering-decomposition" "nixos-init" "taste" ];

  mkSkillFile = homeConfig: name:
    lib.nameValuePair ".claude/skills/${name}" {
      source = homeConfig.config.lib.file.mkOutOfStoreSymlink "${homeConfig.config.home.homeDirectory}/.agents/skills/${name}";
    };

  mkHomeFiles = user:
    lib.mkIf user.homeManager.enable {
      home-manager.users.${user.username} = homeConfig: {
        home.file = lib.listToAttrs (map (mkSkillFile homeConfig) skillNames);
      };
    };
in
{
  config = lib.mkIf (config.settings.apps.enable && config.settings.apps.dev.enable && config.settings.apps.claude.enable) (lib.mkMerge [
    (mkHomeFiles config.settings.users.admin)
    (lib.mkIf config.settings.users.agent.enable (mkHomeFiles config.settings.users.agent))
  ]);
}
