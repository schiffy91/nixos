{ settings, lib, ... }:
let
  clients = lib.optional settings.apps.agents.enable "agents"
    ++ lib.optional settings.apps.claude.enable "claude"
    ++ lib.optional settings.apps.codex.enable "codex"
    ++ lib.optional settings.apps.gemini.enable "gemini";
  skills = import ./skills.nix;
  directories = {
    agents = ".agents/skills";
    codex = ".codex/skills";
    claude = ".claude/skills";
    gemini = ".gemini/config/skills";
  };
  ruleFiles = {
    codex = ".codex/AGENTS.md";
    claude = ".claude/CLAUDE.md";
    gemini = ".gemini/GEMINI.md";
  };
  skillFiles = lib.listToAttrs (lib.concatMap (client:
    map (name: lib.nameValuePair "${directories.${client}}/${name}" { source = skills.${name}; }) settings.apps.agents.skills
  ) clients);
  instructions = lib.listToAttrs (map (client:
    lib.nameValuePair ruleFiles.${client} { source = ./rules.md; }
  ) (lib.filter (client: builtins.hasAttr client ruleFiles) clients));
in lib.mkIf (settings.apps.enable && settings.apps.dev.enable) {
  home.file = skillFiles
    // lib.optionalAttrs settings.apps.agents.rules instructions;
}
