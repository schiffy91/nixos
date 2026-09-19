{ config, pkgs, lib, ... }:
let
  user = config.settings.users.admin.username;
  group = config.users.users.${user}.group;
  home = "/home/${user}";
  protonDir = "${home}/Games/proton";
  compatDir = "${home}/.local/share/Steam/compatibilitytools.d";
  enabled = config.settings.apps.enable && config.settings.apps.gaming.enable && config.settings.apps.steam.enable;
  protonCustom = pkgs.callPackage ./custom/package.nix {
    inherit (pkgs) makeWrapper rsync unzip;
  };
  path = "${protonDir}/${protonCustom.pname}";  # canonical, persisted with ~/Games
  steamPath = "${compatDir}/${protonCustom.pname}";
in lib.mkMerge [
  {
    # Export both locations so anything in the OS can launch this build.
    _module.args.protonCustom = {
      name = protonCustom.pname;
      package = protonCustom;
      inherit path steamPath;
    };
  }
  (lib.mkIf enabled {
    # Steam still needs it discoverable under its own tools directory.
    programs.steam.extraCompatPackages = [ protonCustom ];

    system.activationScripts.protonCustomCompatTool = lib.stringAfter [ "users" ] ''
      export PATH="${pkgs.coreutils}/bin:$PATH"
      link_tool() {  # replace our symlink, preserve anything installed by hand
        target="$1"
        if [ -L "$target" ] || [ ! -e "$target" ]; then
          ln -sfn "${protonCustom}" "$target"
        else
          backup="$target.manual-backup"
          [ -e "$backup" ] && backup="$backup.$(date +%s)"
          mv "$target" "$backup"
          ln -s "${protonCustom}" "$target"
        fi
        chown -h ${user}:${group} "$target"
      }

      install -d -o ${user} -g ${group} "${protonDir}" "${compatDir}"
      link_tool "${path}"
      link_tool "${steamPath}"
    '';
  })
]
