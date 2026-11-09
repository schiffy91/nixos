{ lib }:
{ pkgs, system, configurations }:
let
  desktops = lib.filter (entry:
    entry.configuration.config.settings.desktop.enable
    && entry.configuration.config.settings.users.admin.homeManager.enable
  ) configurations;
  checks = map (entry:
    let
      config = entry.configuration.config;
      home = config.home-manager.users.${config.settings.users.admin.username};
      plasma = home.programs.plasma;
      scripts = lib.filterAttrs (_: script: !script.runAlways) plasma.startup.desktopScript;
      markers = map (name: "${home.xdg.dataHome}/plasma-manager/last_run_desktop_script_${name}")
        (builtins.attrNames scripts);
      persisted = config.settings.disk.immutability.persist.paths;
      missing = lib.filter (path: !(lib.elem path persisted)) markers;
    in assert lib.assertMsg (missing == [])
      "${entry.name}: Plasma startup markers must survive with the desktop layout: ${lib.concatStringsSep ", " missing}";
      ''
        test -f ${lib.escapeShellArg config.settings.desktop.plasma.wallpaper}
        test -f ${lib.escapeShellArg "${config.settings.desktop.cursor.package}/share/icons/${config.settings.desktop.cursor.theme}/index.theme"}
      ''
  ) desktops;
in {
  name = "plasma-startup";
  value = pkgs.runCommand "plasma-startup-${system}" {} ''
    ${lib.concatStringsSep "\n" checks}
    touch "$out"
  '';
}
