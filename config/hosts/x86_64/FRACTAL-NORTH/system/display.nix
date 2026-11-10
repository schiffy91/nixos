{ config, host, lib, pkgs, ... }:
let
  graphics = config.settings.graphics;
  ports = graphics.connectors.${graphics.display}.DP;
  xdr = builtins.head ports;
  tunneled = graphics.display == "integrated";  # its USB4 path adds a 640x480 connector
  # Match the desktop's physical outputs without importing saved virtual
  # displays or other monitors.
  greeterOutputs = (pkgs.formats.json {}).generate "sddm-kwinoutputconfig.json" [
    {
      name = "outputs";
      data = [
        {
          connectorName = xdr;
          edidHash = "6a115673828eb7a99d4101a0041d68a7";
          edidIdentifier = "APP 44590 923665417 1 2019 0";
          scale = host.display.primary.scaleFactor;
          transform = "Normal";
          mode = {
            flags = 1;
            cvt = {
              clock = 1286010;
              flags = 9;
              hdisplay = 6016;
              hsyncStart = 6024;
              hsyncEnd = 6056;
              htotal = 6096;
              hskew = 0;
              vdisplay = 3384;
              vsyncStart = 3502;
              vsyncEnd = 3510;
              vtotal = 3516;
              vscan = 0;
            };
          };
          highDynamicRange = true;
          wideColorGamut = true;
          hdrColorProfileSource = "EDID";
          colorProfileSource = "sRGB";
          colorPowerTradeoff = "PreferAccuracy";
          brightness = 0.56;
          sdrBrightness = 1600;
          maxPeakBrightnessOverride = 1600;
          maxBitsPerColor = 10;
          sdrGamutWideness = 0;
          rgbRange = "Automatic";
          vrrPolicy = "Automatic";
        }
      ] ++ lib.optional tunneled {
        connectorName = builtins.elemAt ports 1;
        edidHash = "e944598217cde35ab667c55081ecbaf8";
        edidIdentifier = "APP 44590 923665417 1 2019 0";
        scale = 1;
        transform = "Normal";
      };
    }
    {
      name = "setups";
      data = [{
        lidClosed = false;
        outputs = [
          { outputIndex = 0; enabled = true; priority = 1; position = { x = 0; y = 0; }; }
        ] ++ lib.optional tunneled { outputIndex = 1; enabled = false; priority = 2; position = { x = 1920; y = 0; }; };
      }];
    }
  ];
  greeterKwin = (pkgs.formats.ini {}).generate "sddm-kwinrc" {
    Compositing.AllowTearing = false;
    Wayland.DisableTonemapping = true;
    Xwayland.Scale = host.display.primary.scaleFactor;
  };
  greeterHome = config.users.users.sddm.home;
in {
  settings.desktop.outputs = [{
    name = xdr;
    scaleFactor = host.display.primary.scaleFactor;
    primary = true;
  }];

  # Install writable copies before KWin starts; it saves output state on exit.
  # Reapply on each greeter start so a reset root cannot lose the display setup.
  systemd.services.display-manager.preStart = lib.mkIf config.services.displayManager.sddm.enable ''
    ${pkgs.coreutils}/bin/install -d -m 0700 -o sddm -g sddm ${lib.escapeShellArg "${greeterHome}/.config"}
    ${pkgs.coreutils}/bin/install -m 0600 -o sddm -g sddm ${greeterOutputs} ${lib.escapeShellArg "${greeterHome}/.config/kwinoutputconfig.json"}
    ${pkgs.coreutils}/bin/install -m 0600 -o sddm -g sddm ${greeterKwin} ${lib.escapeShellArg "${greeterHome}/.config/kwinrc"}
  '';

  # The XDR's internal control device exposes a serial interface; keep ModemManager from probing it.
  services.udev.extraRules = ''
    SUBSYSTEM=="tty", ATTRS{idVendor}=="05ac", ATTRS{idProduct}=="9243", ENV{ID_MM_DEVICE_IGNORE}="1"
  '';
}
