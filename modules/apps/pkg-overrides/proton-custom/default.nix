{ config, pkgs, lib, ... }:
let
  user = config.settings.user.admin.username;
  home = "/home/${user}";
  compatDir = "${home}/.local/share/Steam/compatibilitytools.d";
  enabled = config.settings.apps.enable && config.settings.apps.steam.enable;
  protonCustom = pkgs.callPackage ./package.nix {
    inherit (pkgs) makeWrapper rsync unzip;
  };
in lib.mkMerge [
  {
    # Export the tool name so other modules can reference it.
    _module.args.protonCustom = {
      name = "proton-custom-GE-Proton10-34";
      package = protonCustom;
    };
  }
  (lib.mkIf enabled {
    # Install the compat tool into Steam's compatibilitytools.d via the
    # programs.steam.extraCompatPackages option so Steam discovers it.
    programs.steam.extraCompatPackages = [ protonCustom ];

    # The standalone Battle.net wrapper launches Proton through PROTONPATH, so
    # keep the user-visible compat-tool name pinned to this exact Nix build.
    system.activationScripts.protonCustomCompatTool = lib.stringAfter [ "users" ] ''
      compat_dir="${compatDir}"
      tool_path="$compat_dir/${protonCustom.pname}"
      ${pkgs.coreutils}/bin/install -d -o ${user} -g users "$compat_dir"

      if [ -L "$tool_path" ]; then
        ${pkgs.coreutils}/bin/ln -sfn "${protonCustom}" "$tool_path"
      elif [ -e "$tool_path" ]; then
        backup="$tool_path.manual-backup"
        if [ -e "$backup" ]; then
          backup="$tool_path.manual-backup.$(${pkgs.coreutils}/bin/date +%s)"
        fi
        ${pkgs.coreutils}/bin/mv "$tool_path" "$backup"
        ${pkgs.coreutils}/bin/ln -s "${protonCustom}" "$tool_path"
      else
        ${pkgs.coreutils}/bin/ln -s "${protonCustom}" "$tool_path"
      fi

      ${pkgs.coreutils}/bin/chown -h ${user}:users "$tool_path"
    '';
  })
]
