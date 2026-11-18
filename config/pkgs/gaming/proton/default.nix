{ config, pkgs, lib, inputs, ... }:
let
  user = config.settings.users.admin.username;
  group = config.users.users.${user}.group;
  home = "/home/${user}";
  protonDir = "${home}/Games/proton";
  compatDir = "${home}/.local/share/Steam/compatibilitytools.d";
  enabled = config.settings.apps.gaming.steam.protonSge.enable;
  protonSge = inputs.proton-sge.packages.${pkgs.stdenv.hostPlatform.system}.proton-sge.override {  # schiffy91/proton-sge
    inherit (config.settings.apps.gaming.steam.protonSge) extraFiles;
  };
  path = "${protonDir}/${protonSge.pname}";  # canonical, persisted with ~/Games
  steamPath = "${compatDir}/${protonSge.pname}";
in lib.mkMerge [
  {
    # Export both locations so anything in the OS can launch this build.
    _module.args.protonSge = {
      name = protonSge.pname;
      package = protonSge;
      inherit path steamPath;
    };
  }
  (lib.mkIf enabled {
    # Steam still needs it discoverable under its own tools directory.
    programs.steam.extraCompatPackages = [ protonSge ];

    system.activationScripts.protonSgeCompatTool = lib.stringAfter [ "userFolder" ] ''
      export PATH="${pkgs.coreutils}/bin:$PATH"
      link_tool() {  # replace our symlink, preserve anything installed by hand
        target="$1"
        if [ -L "$target" ] || [ ! -e "$target" ]; then
          ln -sfn "${protonSge}" "$target"
        else
          backup="$target.manual-backup"
          [ -e "$backup" ] && backup="$backup.$(date +%s)"
          mv "$target" "$backup"
          ln -s "${protonSge}" "$target"
        fi
        chown -h ${user}:${group} "$target"
      }

      user_folder "${protonDir}"
      user_folder "${compatDir}"
      link_tool "${path}"
      link_tool "${steamPath}"
    '';
  })
]
