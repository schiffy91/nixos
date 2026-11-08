{ config, lib, pkgs, inputs, steam, protonCustom, winePrefixDpi, gameLaunch, ... }:
let
  enabled = config.settings.apps.gaming.assettocorsa.enable;
  wheelLauncher = lib.optionalString config.settings.apps.gaming.simracing.enable
    "${inputs.logitech-trueforce.packages.${pkgs.stdenv.hostPlatform.system}.logi-wheel}/bin/logi-launch";
  package = import ./package.nix {
    inherit pkgs inputs wheelLauncher;
    mods = config.settings.apps.gaming.assettocorsa.mods;
    csp = config.settings.apps.gaming.assettocorsa.csp;
    contentDirectory = config.settings.apps.gaming.assettocorsa.contentDirectory;
    presets = config.settings.apps.gaming.assettocorsa.presets;
    controlBindings = config.settings.apps.gaming.assettocorsa.controlBindings;
    brakeGamma = config.settings.apps.gaming.assettocorsa.brakeGamma;
    gameSettings = config.settings.apps.gaming.assettocorsa.gameSettings;
    wheelMenu = config.settings.apps.gaming.assettocorsa.wheelMenu.enable;
    uiScale = if config.settings.apps.gaming.assettocorsa.uiScale == null
      then lib.min 2.0 (lib.max 1.0 (winePrefixDpi.dpi / 96.0))
      else config.settings.apps.gaming.assettocorsa.uiScale;
  };
  steamConfig = {
    compatTool = protonCustom.name;
    launchOptions = "${winePrefixDpi.launchPrefix} ${package}/bin/assetto launch -- %command%";
    steamInput = config.settings.apps.gaming.assettocorsa.steamInput;
  };
  appConfig = pkgs.writeText "assetto-steam.json" (builtins.toJSON {
    "244210" = steamConfig;
  });
  setup = pkgs.writeShellApplication {
    name = "assetto-setup";
    runtimeInputs = [ pkgs.procps package ];
    text = ''
      if pgrep -x steam >/dev/null; then
        echo "Let the downloads finish, then exit Steam before running assetto-setup." >&2
        exit 1
      fi
      ${lib.optionalString (config.settings.apps.gaming.assettocorsa.contentDirectory != null) ''
      directory=${let directory = config.settings.apps.gaming.assettocorsa.contentDirectory; in
        if lib.hasPrefix "~/" directory then ''"$HOME"/${lib.escapeShellArg (lib.removePrefix "~/" directory)}''
        else lib.escapeShellArg directory}
      mkdir -p -- "$directory"
      ''}
      assetto apply
      ${steam.configureSteamApps}/bin/configure-steam-apps \
        --steam-path "$HOME/.local/share/Steam" --app-id 244210 \
        --launch-wrapper ${lib.escapeShellArg gameLaunch.prefix} \
        --default-tool ${protonCustom.name} --app-config ${appConfig}
      echo "Ready. Open Steam and press Play on Assetto Corsa to launch Content Manager."
    '';
  };
in {
  imports = [ ./profiles.nix ];
  config = lib.mkMerge [ {
    _module.args.assetto = {
      inherit enabled package wheelLauncher steamConfig;
    };
  } (lib.mkIf enabled {
    environment.systemPackages = [ package setup ];
    fonts.packages = [ pkgs.corefonts ];
    system.build.assetto = package;
    system.build.assetto-setup = setup;
  }) (lib.mkIf (enabled && config.settings.users.admin.homeManager.enable && config.settings.apps.gaming.assettocorsa.followLauncher) {
    home-manager.users.${config.settings.users.admin.username} = {
      xdg.dataFile."kwin/scripts/assetto-follow-cm".source = import ./kwin/package.nix { inherit pkgs; };
      programs.plasma.configFile.kwinrc.Plugins."assetto-follow-cmEnabled" = true;
    };
  })
  ];
}
