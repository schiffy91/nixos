{ settings, lib, ... }:
let
  clients = lib.optional settings.apps.dev.agents.enable "agents"
    ++ lib.optional settings.apps.dev.claude.enable "claude"
    ++ lib.optional settings.apps.dev.codex.enable "codex"
    ++ lib.optional settings.apps.dev.gemini.enable "gemini";
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
    map (name: lib.nameValuePair "${directories.${client}}/${name}" { source = skills.${name}; }) settings.apps.dev.agents.skills
  ) clients);
  instructions = lib.listToAttrs (map (client:
    lib.nameValuePair ruleFiles.${client} { source = ./rules.md; }
  ) (lib.filter (client: builtins.hasAttr client ruleFiles) clients));
in {
  home.file = skillFiles
    // lib.optionalAttrs settings.apps.dev.agents.rules instructions;
}
