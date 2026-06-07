{ config, lib, ... }:
let
  skills = {
    engineering-decomposition = ./agents/skills/engineering-decomposition;
    nixos-init = ./agents/skills/nixos-init;
    taste = ./agents/skills/taste;
  };

  skillFiles = lib.mapAttrs' (name: source:
    lib.nameValuePair ".agents/skills/${name}" { inherit source; }
  ) skills;

  mkHomeFiles = user:
    lib.mkIf user.homeManager.enable {
      home-manager.users.${user.username}.home.file = skillFiles;
    };
in
{
  config = lib.mkIf (config.settings.apps.enable && config.settings.apps.dev.enable && config.settings.apps.agents.enable) (lib.mkMerge [
    (mkHomeFiles config.settings.users.admin)
    (lib.mkIf config.settings.users.agent.enable (mkHomeFiles config.settings.users.agent))
  ]);
}
