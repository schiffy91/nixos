{ lib }:
{ pkgs, system, configurations }:
let
  base = (lib.findFirst (entry:
    lib.hasPrefix "QEMU-" entry.name && lib.hasSuffix "-Standard-Boot" entry.name)
    (lib.head configurations) configurations).configuration;
  hostPath = "/var/lib/persistence-host-extra";
  fixture = overrides: (base.extendModules {
    modules = [({ config, ... }: {
      settings = lib.recursiveUpdate {
        apps = {
          enable = lib.mkForce true;
          utils.desktop.enable = lib.mkDefault config.settings.apps.utils.enable;
        };
        desktop.enable = lib.mkForce true;
        users.admin.homeManager.enable = lib.mkForce true;
        users.agent.enable = lib.mkForce true;
        disk = {
          persistence.enable = lib.mkForce true;
          immutability = {
            enable = lib.mkForce true;
            persist.paths = [ hostPath ];
          };
        };
      } overrides;
    })];
  }).config;
  enabled = fixture {};
  home = enabled.settings.users.admin.homeDirectory;
  agentHome = enabled.users.users.${enabled.settings.users.agent.username}.home;
  paths = config: config.settings.disk.immutability.persist.paths;
  includes = expected: config: lib.all (path: lib.elem path (paths config)) expected;
  excludes = expected: config: lib.all (path: !(lib.elem path (paths config))) expected;
  core = [ "/etc/nixos" "/etc/machine-id" hostPath ];
  rclonePath = "${home}/.config/rclone";
  chromePath = "${home}/.config/google-chrome";
  sunshinePath = "${home}/.config/sunshine";
  mullvadPaths = [ "/etc/mullvad-vpn/" "${home}/.config/Mullvad VPN" ];
  sharedDesktop = map (path: "${home}/${path}") [
    ".local/share/applications" ".local/share/icons" ".local/share/desktop-directories"
    ".local/share/flatpak/db" ".config/dconf/user" ".config/kdeglobals"
  ];
  noRclone = fixture { apps.utils.rclone.enable = lib.mkForce false; };
  noUtils = fixture { apps.utils.enable = lib.mkForce false; };
  noGaming = fixture { apps.gaming.enable = lib.mkForce false; };
  noApps = fixture { apps.enable = lib.mkForce false; };
  noDesktopApps = fixture {
    apps.utils.chrome.enable = lib.mkForce false;
    apps.gaming.sunshine.enable = lib.mkForce false;
  };
  noAgent = fixture { users.agent.enable = lib.mkForce false; };
  customHome = "/srv/persistence-admin";
  relocated = fixture { users.admin.homeDirectory = lib.mkForce customHome; };
  retained = fixture {
    apps.utils.rclone.enable = lib.mkForce false;
    disk.immutability.persist.paths = [ hostPath rclonePath ];
  };
  persistenceOnly = fixture { disk.immutability.enable = lib.mkForce false; };
  check = condition: message: lib.optional (!condition) message;
  failures =
    check (base.options.settings.disk.immutability.persist.paths.default == [])
      "The persistence schema must have an empty default; contributors own the paths."
    ++ check (includes (core ++ [ rclonePath chromePath sunshinePath ] ++ mullvadPaths) enabled)
      "Core, enabled packages and a host contribution must all survive the list merge."
    ++ check (includes core noRclone && excludes [ rclonePath ] noRclone)
      "Disabling rclone must remove its state without removing core or host state."
    ++ check (includes core noUtils && excludes ([ rclonePath chromePath ] ++ mullvadPaths) noUtils)
      "Disabling utils must remove its package state while retaining core and host state."
    ++ check (includes core noGaming && excludes [ sunshinePath ] noGaming)
      "Disabling gaming must remove Sunshine state while retaining core and host state."
    ++ check (includes core noApps && excludes ([ rclonePath chromePath sunshinePath ] ++ mullvadPaths) noApps)
      "Disabling apps must remove all package contributions while retaining core and host state."
    ++ check (includes sharedDesktop noDesktopApps && excludes [ chromePath sunshinePath ] noDesktopApps)
      "Shared desktop state must survive disabling Chrome and Sunshine."
    ++ check (includes sharedDesktop noApps)
      "Shared desktop persistence belongs to the desktop, independently of package groups."
    ++ check (includes [ "${agentHome}/work" ] enabled
      && excludes enabled.settings.users.agent.persist.paths noAgent)
      "Agent enablement must control the agent's persistence contribution."
    ++ check (includes [
      "${customHome}/Downloads" "${customHome}/.config/rclone"
      "${customHome}/.config/sunshine" "${customHome}/.config/google-chrome"
      "${customHome}/.config/Mullvad VPN" "${customHome}/.local/share/applications"
    ] relocated && !(lib.any (lib.hasPrefix "${home}/") (paths relocated)))
      "Admin persistence must follow homeDirectory, including shared desktop and package paths."
    ++ check (includes (core ++ [ rclonePath ]) retained
      && !(retained.systemd.user.services ? rclone-drive))
      "A host can explicitly retain state after disabling the owning feature."
    ++ check (paths persistenceOnly == paths enabled
      && lib.elem "local-fs.target" persistenceOnly.systemd.services.persistence-mounts.wantedBy
      && persistenceOnly.environment.etc ? "immutability/spec.toml")
      "Persistence must remain active when immutability resets are disabled.";
in {
  name = "persistence-ownership";
  value = pkgs.runCommand "persistence-ownership-${system}" {} (
    if failures == [] then "touch $out"
    else throw (lib.concatStringsSep "\n" failures));
}
