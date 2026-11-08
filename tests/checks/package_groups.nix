{ lib }:
{ pkgs, configurations, ... }:
let
  find = name: lib.findFirst (entry: entry.name == name) null configurations;
  fractal = find "FRACTAL-NORTH-Standard-Boot";
  deck = find "STEAM-DECK-OLED-Standard-Boot";
  check = condition: message: lib.optional (!condition) message;
  failures = lib.optionals (fractal != null && deck != null) (let
    desktop = fractal.configuration;
    handheld = deck.configuration.config;
    disabled = group: (desktop.extendModules {
      modules = [{ settings.apps.${group}.enable = lib.mkForce false; }];
    }).config;
    noGaming = disabled "gaming";
    noDev = disabled "dev";
    noUtils = disabled "utils";
    noLmstudio = disabled "lmstudio";
    deckWithout = disabledOptions: (deck.configuration.extendModules {
      modules = [{ settings.apps = disabledOptions; }];
    }).config;
    disabledDecks = [
      (deckWithout { enable = lib.mkForce false; })
      (deckWithout { gaming.enable = lib.mkForce false; })
      (deckWithout { steam.enable = lib.mkForce false; })
    ];
    noDecky = (deck.configuration.extendModules {
      modules = [{ jovian.decky-loader.enable = lib.mkForce false; }];
    }).config;
    names = config: map lib.getName config.environment.systemPackages;
    desktopOnly = config: let
      home = config.home-manager.users.${config.settings.users.admin.username};
    in !config.programs.steam.enable && !config.jovian.steam.enable
      && !config.jovian.steam.autoStart && !config.jovian.decky-loader.enable
      && config.settings.desktop.defaultSession == "plasma"
      && !config.settings.sleep.lockGamingSession
      && !(home.home.file ? "Desktop/Return to Gaming Mode.desktop")
      && !(home.home.file ? ".local/share/kwin/scripts/steam-keyboard-fit")
      && !(builtins.elem "steamos-session-select" (names config))
      && !(builtins.elem "applications:return-to-gaming-mode.desktop" config.settings.desktop.plasma.taskbarLaunchers);
    # NixOS applies graphics-library and compatibility-tool overrides after selecting this definition.
    steamDefinitions = deck.configuration.options.programs.steam.package.definitionsWithLocations;
  in
    check (!noGaming.programs.steam.enable && !noGaming.services.sunshine.enable
      && !(noGaming.systemd.user.services ? semu-sync))
      "Disabling gaming must disable Steam, Sunshine and Semu services."
    ++ check (!noDev.programs.zsh.enable && !(builtins.elem "claude-code" (names noDev))
      && !(builtins.elem "codex" (names noDev)) && !(builtins.elem "lmstudio" (names noDev)))
      "Disabling dev must disable its shell and shared development packages."
    ++ check (builtins.elem "lmstudio" (names desktop.config)
      && !(builtins.elem "lmstudio" (names noLmstudio)))
      "LM Studio's feature switch must control the shared Linux installation."
    ++ check (!(builtins.elem "google-chrome" (names noUtils)) && !noUtils.services.mullvad-vpn.enable)
      "Disabling utils must disable Chrome and the shared desktop utility services."
    ++ check (desktop.config.settings.apps.chrome.flags == handheld.settings.apps.chrome.flags
      && desktop.config.settings.apps.chrome.renderNode == "/dev/dri/by-path/pci-0000:01:00.0-render"
      && handheld.settings.apps.chrome.renderNode == null)
      "Chrome must share common flags while keeping the NVIDIA renderer on FRACTAL-NORTH."
    ++ check (handheld.settings.apps.semu.paths.roms == "/run/media/alexanderschiffhauer/SD/Emulation/ES-DE/ES-DE/ROMs")
      "The shared Semu root must preserve the Deck's SD-card ROM directory."
    ++ check (handheld.settings.apps.sunshine.capture == "auto" && desktop.config.settings.apps.sunshine.capture == "kwin")
      "Sunshine must preserve the Deck's automatic session capture and desktop KWin capture."
    ++ check (lib.length steamDefinitions == 1
      && (lib.head steamDefinitions).value.drvPath == deck.configuration.pkgs.jovianPackages.steam.drvPath)
      "Shared Steam defaults must preserve Jovian's specialized Deck package."
    ++ check (lib.all desktopOnly disabledDecks)
      "Disabling apps, gaming or Steam must leave the Deck in Plasma without gaming session integrations."
    ++ check (builtins.elem "/var/lib/decky-loader" handheld.settings.disk.immutability.persist.paths
      && noDecky.jovian.steam.enable && !noDecky.jovian.decky-loader.enable
      && !(builtins.elem "/var/lib/decky-loader" noDecky.settings.disk.immutability.persist.paths))
      "Disabling Decky independently of Steam must remove its persistence contribution."
  );
in {
  name = "package-groups";
  value = pkgs.runCommand "package-groups" {} (
    if failures == [] then "touch $out"
    else "echo ${lib.escapeShellArg (lib.concatStringsSep "; " failures)} >&2; exit 1"
  );
}
