{ config, lib, ... }:
let
  apps = config.settings.apps;
  home = config.settings.users.admin.homeDirectory;
in {
  settings.disk.immutability.persist.paths =
    lib.optional apps.dev.codex.enable "${home}/.codex"
    ++ lib.optionals apps.dev.claude.enable [ "${home}/.claude" "${home}/.claude.json" ]
    ++ lib.optional apps.dev.gemini.enable "${home}/.gemini";
}
