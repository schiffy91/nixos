{ lib }:
{ pkgs, ... }:
let
  evaluate = modules: (lib.evalModules {
    specialArgs = { inherit pkgs; };
    modules = [
      ../../config/settings.nix
      { options.assertions = lib.mkOption { type = lib.types.listOf lib.types.attrs; default = []; }; }
    ] ++ modules;
  }).config;
  failures = config: map (entry: entry.message) (lib.filter (entry: !entry.assertion) config.assertions);
  valid = modules: failures (evaluate modules) == [];
  rejects = child: parent: modules:
    lib.any (message: lib.hasInfix child message && lib.hasInfix parent message)
      (failures (evaluate modules));
  check = condition: message: lib.optional (!condition) message;
  rootOff = evaluate [{ settings.apps.enable = false; }];
  groupsOff = evaluate [{ settings.apps = { dev.enable = false; gaming.enable = false; utils.enable = false; }; }];
  chromeApp = { settings.apps.utils.chrome.webApps.mail.url = "https://mail.example.com/"; };
  errors =
    check (valid []) "Default app settings must have a consistent hierarchy."
    ++ check (failures rootOff == [] && !rootOff.settings.apps.dev.vscode.enable
      && !rootOff.settings.apps.gaming.steam.protonCustom.enable
      && !rootOff.settings.apps.utils.nixosctl.caffeine.enable)
      "Defaults must follow a disabled root through every level."
    ++ check (failures groupsOff == [] && !groupsOff.settings.apps.dev.git.enable
      && !groupsOff.settings.apps.gaming.sunshine.enable && !groupsOff.settings.apps.utils.drive.enable)
      "Disabling groups without explicit child overrides must be valid."
    ++ check (rejects "settings.apps.dev.vscode.enable" "settings.apps.dev.enable" [{
      settings.apps.dev = { enable = false; vscode.enable = true; };
    }]) "An explicitly enabled app under a disabled group must be rejected."
    ++ check (rejects "settings.apps.dev.vscode.enable" "settings.apps.dev.enable" [{
      settings.apps.dev = { enable = lib.mkForce false; vscode.enable = lib.mkForce true; };
    }]) "Override priority must not bypass hierarchy validation."
    ++ check (rejects "settings.apps.dev.enable" "settings.apps.enable" [{
      settings.apps = { enable = false; dev.enable = true; };
    }]) "An explicitly enabled group under a disabled root must be rejected."
    ++ check (rejects "settings.apps.gaming.sunshine.sshLogin.enable" "settings.apps.gaming.sunshine.enable" [{
      settings.apps.gaming.sunshine = { enable = false; sshLogin.enable = true; };
    }]) "Nested features must validate their nearest enabled ancestor."
    ++ check (rejects "settings.apps.utils.chrome.webApps.mail.enable" "settings.apps.utils.chrome.enable" [ chromeApp {
      settings.apps.utils.chrome = { enable = false; webApps.mail.enable = true; };
    }]) "Dynamically named PWAs must participate in hierarchy validation."
    ++ check (valid [ chromeApp { settings.apps.utils.chrome.enable = false; } ])
      "A PWA declaration without an explicit enable must follow Chrome's default."
    ++ check (rejects "settings.apps.dev.example.enable" "settings.apps.dev.enable" [{
      options.settings.apps.dev.example.enable = lib.mkEnableOption "test app";
      config.settings.apps.dev = { enable = false; example.enable = true; };
    }]) "New apps must be validated without adding them to a separate registry."
    ++ check (rejects "settings.apps.dev.example.enable" "settings.apps.dev.enable" [{
      options.settings.apps.dev.example = lib.mkOption {
        default = {};
        type = lib.types.submodule { options.enable = lib.mkEnableOption "test app"; };
      };
      config.settings.apps.dev = { enable = false; example.enable = true; };
    }]) "Apps declared as submodules must participate in the same hierarchy."
    ++ check (valid [{ settings.apps.utils.nixosctl.settings.plugin = { enable = false; nested.enable = true; }; }])
      "Application payloads must not be mistaken for feature declarations."
    ++ check (rejects "settings.apps.gaming.rocksmith.enable" "settings.apps.gaming.steam.protonCustom.enable" [{
      settings.apps.gaming = { steam.enable = false; rocksmith.enable = true; };
    }]) "Sibling dependencies must reject unsupported combinations."
    ++ check (valid [{ settings.apps.gaming.steam.enable = false; }])
      "Default Steam dependents must turn off when Steam is disabled.";
in {
  name = "app-hierarchy";
  value = pkgs.runCommand "app-hierarchy" {} (
    if errors == [] then "touch $out" else throw (lib.concatStringsSep "\n" errors));
}
