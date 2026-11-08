{ config, pkgs, lib, inputs, ... }:
let
  virtualDisplay = config.settings.apps.sunshine.virtualDisplay;
  enabled = config.settings.apps.enable && config.settings.apps.gaming.enable && config.settings.apps.sunshine.enable && virtualDisplay.enable;
  primary = lib.findFirst (output: output.primary) null config.settings.desktop.outputs;
  primaryOutput =
    if virtualDisplay.primaryOutput != ""
    then virtualDisplay.primaryOutput
    else if primary != null
    then primary.name
    else "DP-1";
  virtualOutput = "Virtual-${virtualDisplay.name}";
  configuration = pkgs.writeText "sunshine-virtual-display.json" (builtins.toJSON {
    inherit (virtualDisplay) name defaultWidth defaultHeight defaultFps scale position port settleSeconds disablePrimaryOnStream;
    primary = primaryOutput;
  });
  controller = (import ../lib/build-tools.nix { inherit pkgs inputs; }) {
    name = "sunshine-virtual-display";
    sources = [ ./virtual-display ../../../lib/BackgroundProcess.btrc ];
    programs.sunshine-virtual-display = "sunshine/virtual-display/Main.btrc";
    runtimeInputs = [ pkgs.coreutils pkgs.kdePackages.krfb pkgs.kdePackages.libkscreen ];
  };
  startVirtualDisplay = pkgs.writeShellApplication {
    name = "sunshine-start-virtual-display";
    text = ''exec ${controller}/bin/sunshine-virtual-display ${configuration} start'';
  };
  stopVirtualDisplay = pkgs.writeShellApplication {
    name = "sunshine-stop-virtual-display";
    text = ''exec ${controller}/bin/sunshine-virtual-display ${configuration} stop'';
  };
  stopSteamVirtualDisplay = pkgs.writeShellApplication {
    name = "sunshine-stop-steam-virtual-display";
    runtimeInputs = with pkgs; [
      steam
      util-linux
    ];
    text = ''
      set +e
      ${stopVirtualDisplay}/bin/sunshine-stop-virtual-display
      setsid steam steam://close/bigpicture
      exit 0
    '';
  };
in lib.mkIf enabled {
  services.sunshine = {
    settings.output_name = virtualOutput;
    applications = {
      env.PATH = lib.makeBinPath (with pkgs; [
        coreutils
        steam
        util-linux
      ]);
      apps = [
        {
          name = "Desktop";
          image-path = "desktop.png";
          prep-cmd = [{
            do = "${startVirtualDisplay}/bin/sunshine-start-virtual-display";
            undo = "${stopVirtualDisplay}/bin/sunshine-stop-virtual-display";
          }];
        }
        {
          name = "Steam Big Picture";
          detached = [ "${pkgs.util-linux}/bin/setsid ${pkgs.steam}/bin/steam steam://open/bigpicture" ];
          image-path = "steam.png";
          prep-cmd = [{
            do = "${startVirtualDisplay}/bin/sunshine-start-virtual-display";
            undo = "${stopSteamVirtualDisplay}/bin/sunshine-stop-steam-virtual-display";
          }];
        }
      ];
    };
  };

  environment.systemPackages = [
    pkgs.kdePackages.krfb
    pkgs.kdePackages.libkscreen
    startVirtualDisplay
    stopVirtualDisplay
  ];
}
