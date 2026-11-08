# All Wine launchers share the primary display's declared scale.
{ config, pkgs, lib, inputs, ... }:
let
  primary = lib.findFirst (output: output.primary) null config.settings.desktop.outputs;
  scale = if primary == null then 1.0 else primary.scaleFactor;
  dpi = builtins.floor (96.0 * scale + 0.5);
  tool = import ./package.nix { inherit pkgs inputs; };
in lib.mkMerge [
  {
    _module.args.winePrefixDpi = {
      package = tool;
      inherit dpi;
      launchPrefix = "${tool}/bin/wine-prefix-dpi exec --dpi ${toString dpi} --";
    };
  }
  (lib.mkIf (config.settings.apps.gaming.enable) {
    settings.disk.immutability.persist.paths = [ "${config.settings.users.admin.homeDirectory}/.local/share/umu" ];
    users.users.${config.settings.users.admin.username}.packages = [ tool ];
  })
]
