{ pkgs, inputs, mods ? [], csp ? null, wheelLauncher ? "", contentDirectory ? null, presets ? [], uiScale ? 1.0, controlBindings ? [], brakeGamma ? null, wheelMenu ? false, gameSettings ? {} }:
let
  tools = import ./tools.nix { inherit pkgs inputs; };
  assets = import ./lib.nix { inherit pkgs; };
  presetNames = map (preset: preset.name) presets;
  gameSettingsConfiguration = pkgs.writeText "assetto-game-settings.json" (builtins.toJSON gameSettings);
  bindingConfiguration = pkgs.writeText "assetto-control-bindings.json" (builtins.toJSON controlBindings);
  presetConfiguration = assert pkgs.lib.assertMsg (builtins.length presetNames == builtins.length (pkgs.lib.unique presetNames))
    "Assetto Corsa preset names must be unique";
    pkgs.writeText "assetto-presets.json" (builtins.toJSON (builtins.listToAttrs (map (preset: {
      inherit (preset) name;
      value = preset.source;
    }) presets)));
  payload = pkgs.runCommand "assettocorsa-mods" {} ''
    ${tools}/bin/assetto merge "$out" ${pkgs.lib.escapeShellArgs (map toString (
      [ assets.contentManager (if csp == null then assets.csp else csp) assets.fonts ]
      ++ pkgs.lib.optional wheelMenu (import ./wheel-menu/package.nix { inherit pkgs; }) ++ mods
    ))}
  '';
in pkgs.runCommand "assetto" { nativeBuildInputs = [ pkgs.makeWrapper ]; } ''
  mkdir -p "$out/bin"
  makeWrapper ${tools}/bin/assetto "$out/bin/assetto" \
    --set ASSETTO_MODS ${payload} \
    --set ASSETTO_CONTENT_DIRECTORY ${pkgs.lib.escapeShellArg (if contentDirectory == null then "" else contentDirectory)} \
    --set ASSETTO_PRESETS ${presetConfiguration} \
    --set ASSETTO_CONTROL_BINDINGS ${bindingConfiguration} \
    --set ASSETTO_GAME_SETTINGS ${gameSettingsConfiguration} \
    --set ASSETTO_BRAKE_GAMMA ${pkgs.lib.escapeShellArg (if brakeGamma == null then "" else toString brakeGamma)} \
    --set ASSETTO_UI_SCALE ${pkgs.lib.escapeShellArg (toString uiScale)} \
    --set ASSETTO_WHEEL_LAUNCHER ${pkgs.lib.escapeShellArg wheelLauncher}
  ln -s ${payload} "$out/mods"
''
