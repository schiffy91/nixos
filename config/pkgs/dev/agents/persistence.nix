{ config, lib, ... }:
let
  apps = config.settings.apps;
  home = config.settings.users.admin.homeDirectory;
in lib.mkIf (apps.enable && apps.dev.enable) {
  settings.disk.immutability.persist.paths =
    lib.optional apps.codex.enable "${home}/.codex"
    ++ lib.optionals apps.claude.enable [ "${home}/.claude" "${home}/.claude.json" ]
    ++ lib.optional apps.gemini.enable "${home}/.gemini";
}
