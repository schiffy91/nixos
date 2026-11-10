{ lib, inputs }:
{ pkgs, configurations, ... }:
let
  find = name: lib.findFirst (entry: entry.name == name) null configurations;
  fractal = find "FRACTAL-NORTH-Standard-Boot";
  deck = find "STEAM-DECK-OLED-Standard-Boot";
  check = condition: message: lib.optional (!condition) message;
  failures = lib.optionals (fractal != null && deck != null) (let
    desktop = fractal.configuration;
    handheld = deck.configuration.config;
    disable = import ./disable-apps.nix { inherit lib; };
    disabled = path: (desktop.extendModules {
      modules = [ (lib.setAttrByPath ([ "settings" "apps" ] ++ lib.splitString "." path)
        (disable (lib.getAttrFromPath (lib.splitString "." path) desktop.config.settings.apps))) ]
        ++ lib.optional (path == "gaming.steam") { settings.apps.gaming.assettocorsa.wheelMenu.enable = lib.mkForce false; };
    }).config;
    noGaming = disabled "gaming";
    noDev = disabled "dev";
    noUtils = disabled "utils";
    noLmstudio = disabled "dev.lmstudio";
    noApps = (desktop.extendModules {
      modules = [{ settings.apps = disable desktop.config.settings.apps; }];
    }).config;
    customChrome = (desktop.extendModules {
      modules = [{
        settings.apps.utils.chrome.profile = "Profile 2";
        settings.apps.utils.chrome.webApps.notes = { name = "Notes"; url = "https://notes.example.com/"; };
      }];
    }).config;
    invalidDev = (desktop.extendModules {
      modules = [{ settings.apps.dev.enable = lib.mkForce false; }];
    }).config;
    chromeLaunchers = [
      "applications:google-chrome.desktop"
      "applications:chrome-app-gmail.desktop"
      "applications:chrome-app-calendar.desktop"
      "applications:chrome-app-whatsapp.desktop"
    ];
    cider = "applications:cider-2.desktop";
    code = "applications:code.desktop";
    battlenet = "applications:battlenet.desktop";
    steam = "applications:steam.desktop";
    expectedLaunchers = [ "applications:org.kde.dolphin.desktop" ] ++ chromeLaunchers
      ++ [ cider code "applications:org.kde.konsole.desktop" battlenet steam ];
    launcherCases = [
      { config = desktop.config; removed = []; }
      { config = noApps; removed = chromeLaunchers ++ [ cider code battlenet steam ]; }
      { config = noUtils; removed = chromeLaunchers ++ [ cider ]; }
      { config = noDev; removed = [ code ]; }
      { config = noGaming; removed = [ battlenet steam ]; }
      { config = disabled "utils.chrome"; removed = chromeLaunchers; }
      { config = disabled "utils.cider"; removed = [ cider ]; }
      { config = disabled "dev.vscode"; removed = [ code ]; }
      { config = disabled "gaming.steam"; removed = [ battlenet steam ]; }
      { config = disabled "gaming.battlenet"; removed = [ battlenet ]; }
      { config = disabled "utils.chrome.webApps.gmail"; removed = [ "applications:chrome-app-gmail.desktop" ]; }
    ];
    deckWithout = path: (deck.configuration.extendModules {
      modules = [ (lib.setAttrByPath ([ "settings" "apps" ] ++ path)
        (disable (lib.getAttrFromPath path handheld.settings.apps))) ];
    }).config;
    disabledDecks = [
      (deckWithout [])
      (deckWithout [ "gaming" ])
      # Battle.net is explicitly enabled on the Deck and requires Proton.
      ((deck.configuration.extendModules { modules = [{
        settings.apps.gaming = {
          steam = disable handheld.settings.apps.gaming.steam;
          battlenet.enable = lib.mkForce false;
        };
      }]; }).config)
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
    ++ check (desktop.config.settings.apps.utils.chrome.flags == [
        "--enable-features=VaapiOnNvidiaGPUs,WaylandWindowDecorations" "--disable-features=WaylandWpColorManagerV1" ]
      && desktop.config.settings.apps.utils.chrome.renderNode == null
      && handheld.settings.apps.utils.chrome.renderNode == null)
      "FRACTAL-NORTH's Chrome must run on native Wayland with NVDEC decode and no render-node override."
    ++ check (lib.all (case:
      case.config.settings.desktop.plasma.taskbarLaunchers == lib.subtractLists case.removed expectedLaunchers)
      launcherCases)
      "FRACTAL-NORTH's taskbar must preserve app order and Chrome identities while following package switches."
    ++ check (lib.all (case: lib.all (entry: entry.assertion) case.config.assertions) launcherCases
      && lib.all (config: lib.all (entry: entry.assertion) config.assertions) disabledDecks)
      "Package-switch fixtures must satisfy the hierarchy and dependency assertions."
    ++ check (!(builtins.tryEval invalidDev.system.build.toplevel.drvPath).success)
      "A host with explicitly enabled dev children must fail system evaluation when dev is disabled."
    ++ check (lib.length (disabled "utils.chrome.webApps.gmail").programs.chromium.extraOpts.WebAppInstallForceList == 2)
      "Disabling a PWA must remove its install policy as well as its taskbar pin."
    ++ check (desktop.config.programs.chromium.extraOpts.WebAppInstallForceList == [
      { url = "https://calendar.google.com/calendar/"; custom_name = "Google Calendar"; default_launch_container = "window"; create_desktop_shortcut = false; }
      { url = "https://mail.google.com/mail/"; custom_name = "Gmail"; default_launch_container = "window"; create_desktop_shortcut = false; }
      { url = "https://web.whatsapp.com/"; custom_name = "WhatsApp"; default_launch_container = "window"; create_desktop_shortcut = false; }
    ]) "Chrome must install native PWAs by URL with stable policy names."
    ++ check ((builtins.fromJSON desktop.config.environment.etc."opt/chrome/policies/managed/extra.json".text).WebAppInstallForceList
      == desktop.config.programs.chromium.extraOpts.WebAppInstallForceList)
      "PWA policy must be written to Google Chrome's managed policy directory."
    ++ check (customChrome.settings.apps.utils.chrome.webApps.notes.launcher == "applications:chrome-app-notes.desktop"
      && customChrome.settings.apps.utils.chrome.webApps.gmail.launcher == desktop.config.settings.apps.utils.chrome.webApps.gmail.launcher
      && builtins.elem {
        url = "https://notes.example.com/"; custom_name = "Notes";
        default_launch_container = "window"; create_desktop_shortcut = false;
      } customChrome.programs.chromium.extraOpts.WebAppInstallForceList)
      "Adding a PWA must generate its policy and stable launcher without a Chrome ID or profile-specific pin."
    ++ check (lib.all (config:
      !(config.programs.chromium.extraOpts ? WebAppInstallForceList)
      && !(config.home-manager.users.${config.settings.users.admin.username}.systemd.user.services ? chrome-launchers))
      [ noApps noUtils (disabled "utils.chrome") ])
      "Disabling Chrome or its package groups must remove PWA installation policy and launcher watching."
    ++ check (handheld.settings.apps.gaming.semu.paths.roms == "/run/media/alexanderschiffhauer/SD/Emulation/ES-DE/ES-DE/ROMs")
      "The shared Semu root must preserve the Deck's SD-card ROM directory."
    ++ check (handheld.settings.apps.gaming.sunshine.capture == "auto" && desktop.config.settings.apps.gaming.sunshine.capture == "kwin")
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
    if failures == [] then ''
      test -e ${import ./chrome-launchers.nix { inherit pkgs inputs; }}
      touch $out
    ''
    else "echo ${lib.escapeShellArg (lib.concatStringsSep "; " failures)} >&2; exit 1"
  );
}
