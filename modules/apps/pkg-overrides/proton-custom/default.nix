{ config, pkgs, lib, ... }:
let
  user = config.settings.user.admin.username;
  home = "/home/${user}";
  compatDir = "${home}/.local/share/Steam/compatibilitytools.d";
  scwhine-proton = pkgs.callPackage ./package.nix {
    inherit (pkgs) makeWrapper rsync unzip;
  };
in {
  # Install the compat tool into Steam's compatibilitytools.d via the
  # programs.steam.extraCompatPackages option so Steam discovers it.
  programs.steam.extraCompatPackages = [ scwhine-proton ];

  # Export the tool name so other modules can reference it.
  _module.args.scwhineProton = {
    name = "scwhine-GE-Proton10-34";
    package = scwhine-proton;
  };

  # The standalone Battle.net wrapper launches Proton through PROTONPATH, so
  # keep the user-visible compat-tool name pinned to this exact Nix build.
  system.activationScripts.scwhineProtonCompatTool = lib.stringAfter [ "users" ] ''
    compat_dir="${compatDir}"
    tool_path="$compat_dir/${scwhine-proton.pname}"
    ${pkgs.coreutils}/bin/install -d -o ${user} -g users "$compat_dir"

    if [ -L "$tool_path" ]; then
      ${pkgs.coreutils}/bin/ln -sfn "${scwhine-proton}" "$tool_path"
    elif [ -e "$tool_path" ]; then
      backup="$tool_path.manual-backup"
      if [ -e "$backup" ]; then
        backup="$tool_path.manual-backup.$(${pkgs.coreutils}/bin/date +%s)"
      fi
      ${pkgs.coreutils}/bin/mv "$tool_path" "$backup"
      ${pkgs.coreutils}/bin/ln -s "${scwhine-proton}" "$tool_path"
    else
      ${pkgs.coreutils}/bin/ln -s "${scwhine-proton}" "$tool_path"
    fi

    ${pkgs.coreutils}/bin/chown -h ${user}:users "$tool_path"
  '';
}
