{ pkgs, inputs, plymouth ? pkgs.plymouth }:
let
  build = import ../../../lib/btrc.nix { inherit pkgs inputs; };
  src = ./helpers;
  blank = build {
    name = "sleep-lock-blank";
    inherit src;
    programs.sleep-lock-blank = "Blank.btrc";
    buildInputs = [ pkgs.libdrm ];
    linkFlags = [ "-D_GNU_SOURCE" "-ldrm" "-I${pkgs.lib.getDev pkgs.libdrm}/include/libdrm" ];
  };
  native = build {
    name = "sleep-lock-native";
    inherit src;
    programs.sleep-lock-native = "Authenticate.btrc";
    buildInputs = [ pkgs.pam ];
    linkFlags = [ "-D_GNU_SOURCE" "-lpam" ];
    wrapperArgs = [ "--add-flags" "${plymouth}/bin/plymouth" ];
  };
in { inherit blank native; }
