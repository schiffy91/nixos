{ config, pkgs, lib, ... }: let
  cfg = config.services.sunshine;
  output = "sunshine-virtual";
  virtualDisplay = pkgs.writeShellApplication {
    name = "sunshine-virtual-display";
    runtimeInputs = with pkgs; [ kdePackages.libkscreen ];
    text = ''
      W="''${SUNSHINE_CLIENT_WIDTH:-1920}"
      H="''${SUNSHINE_CLIENT_HEIGHT:-1080}"
      FPS="''${SUNSHINE_CLIENT_FPS:-60}"
      HDR="''${SUNSHINE_CLIENT_HDR:-0}"
      case "$1" in
        start)
          kscreen-doctor "output.${output}.add" || true
          kscreen-doctor "output.${output}.mode.''${W}x''${H}@''${FPS}"
          kscreen-doctor "output.${output}.enable"
          [ "$HDR" = "1" ] && kscreen-doctor "output.${output}.hdr.enable" "output.${output}.wcg.enable"
          ;;
        stop)
          kscreen-doctor "output.${output}.disable" "output.${output}.remove" || true
          ;;
      esac
    '';
  };
in lib.mkIf cfg.enable {
  services.sunshine.settings = {
    output_name = output;
    global_prep_cmd = builtins.toJSON [{
      do   = "${virtualDisplay}/bin/sunshine-virtual-display start";
      undo = "${virtualDisplay}/bin/sunshine-virtual-display stop";
    }];
  };
}
