{ pkgs, inputs, plymouth ? pkgs.plymouth }:
let
  build = inputs.btrc.lib.buildPackage pkgs;
  src = ./helpers;
  blank = build {
    name = "sleep-lock-blank";
    inherit src;
    programs.sleep-lock-blank = "Blank.btrc";
    buildInputs = [ pkgs.libdrm ];
  };
  native = build {
    name = "sleep-lock-native";
    inherit src;
    programs.sleep-lock-native = "Authenticate.btrc";
    buildInputs = [ pkgs.pam ];
    wrapperArgs = [ "--add-flags" "${plymouth}/bin/plymouth" ];
  };
in { inherit blank native; }
