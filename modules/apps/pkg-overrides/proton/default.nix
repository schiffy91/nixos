{ config, pkgs, lib, ... }:
let
  scwhine-proton = pkgs.callPackage ./package.nix {
    inherit (pkgs) fetchurl autoPatchelfHook makeWrapper rsync unzip;
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
}
