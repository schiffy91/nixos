{ lib, config, pkgs, ... }:
{
  options.settings.apps.gaming.wow = {
    enable = lib.mkOption {
      type = lib.types.bool;
      default = config.settings.apps.gaming.battlenet.enable;
    };
    directory = lib.mkOption {
      type = lib.types.str;
      default = "${config.settings.apps.gaming.battlenet.prefix}/drive_c/Program Files (x86)/World of Warcraft/_forever_";
      description = "WoW client directory containing Interface and WTF. Override for another installation or client edition; select compatible addons separately.";
    };
    addonPackages = lib.mkOption {
      type = lib.types.attrsOf lib.types.package;
      readOnly = true;
      default = import ./addons.nix { inherit pkgs; overrides = config.settings.apps.gaming.wow.addonOverrides; };
      description = "Named addon packages from the shared release lock, each with its `addonOverrides` applied.";
    };
    addonOverrides = lib.mkOption {
      type = lib.types.attrsOf (lib.types.submodule {
        options = {
          patches = lib.mkOption {
            type = lib.types.listOf lib.types.path;
            default = [];
            description = "Patches applied in order from the archive's root (`patch -p1`).";
          };
          files = lib.mkOption {
            type = lib.types.attrsOf lib.types.path;
            default = {};
            description = "Files placed in the addon after the patches: a path inside the archive, and the file put there.";
          };
          tocAppend = lib.mkOption {
            type = lib.types.attrsOf (lib.types.listOf lib.types.str);
            default = {};
            description = "Lines added to the end of a `.toc` file, so the client loads the placed files.";
          };
        };
      });
      default = {};
      description = "Changes made to an addon of the release lock, under its name there. Modules and hosts add to it.";
    };
    addons = lib.mkOption {
      type = lib.types.listOf lib.types.package;
      default = builtins.attrValues config.settings.apps.gaming.wow.addonPackages;
      description = "Addon archives to install. Each package contains top-level addon directories. Use mkForce to replace the default selection.";
    };
    upgradeAddons = lib.mkOption {
      type = lib.types.bool;
      default = true;
      description = "Refresh the addon release pins during `nixosctl upgrade`. Turn off on hosts that take the pins another host committed.";
    };
    settingsExcluded = lib.mkOption {
      type = lib.types.listOf lib.types.str;
      default = [ "Details_DataStorage" ];
      description = "Managed addon directories whose SavedVariables `wow-settings capture` leaves out, such as combat history.";
    };
  };
}
