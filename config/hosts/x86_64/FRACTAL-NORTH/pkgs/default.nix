{ config, pkgs, ... }:
let
  graphics = config.settings.graphics;
  xdr = builtins.head graphics.connectors.${graphics.display}.DP;
  tv = builtins.head graphics.connectors.nvidia."HDMI-A";
in {
  imports = [ ./desktop.nix ];
  settings.apps = {
    dev.packages = with pkgs; [ python3 ];
    gaming.packages = with pkgs; [ protonup-qt ];
    dev.distrobox.enable = true;
    gaming.moonlight.enable = true;
    gaming.deckpad.enable = true;  # attaches the Deck's controller whenever the Deck offers it
    utils.packages = with pkgs; [
      tcpdump ethtool pciutils usbutils sbctl fwupd mesa-demos
      vulkan-tools vdpauinfo libva-utils nvtopPackages.full
    ];
    utils.desktop.enable = true;
    dev.lmstudio.enable = true;
    utils.chrome.flags = [
      "--enable-features=VaapiOnNvidiaGPUs,WaylandWindowDecorations"  # NVDEC decode; restates the wrapper's feature
      "--disable-features=WaylandWpColorManagerV1"  # Chrome blows out HDR video here
    ];
    gaming.steam.extraEnv = graphics.environment;
    gaming.sunshine = {
      capture = "kwin";
      cuda.enable = true;
    };
    gaming.simracing.enable = true;
    gaming.assettocorsa = {
      contentDirectory = "~/Drive/media/Games/Assetto Corsa";
      presetName = "FRACTAL-NORTH";
      graphicsPreset = "pure-lcs-medium";
      wheelProfile = "logitech-pro-playstation";
      brakeGamma = 1.0; # Linear response for the pressure-sensing PRO brake.
      wheelMenu.enable = true;
    };
    utils.nixosctl.settings = {
      displays = { ${xdr} = "ProXDR"; ${tv} = "S89C"; };
      layout = [{ display = tv; position = "left-of"; relative_to = xdr; }];
      audio = [
        {
          label = "Kanto TUK";
          card = "alsa_card.usb-Neural_DSP_Quad_Cortex-00";
          profile = "output:analog-surround-71+input:analog-stereo";
          sink = "alsa_output.usb-Neural_DSP_Quad_Cortex-00.analog-surround-71";
          volume = "100%";
        }
        {
          label = "Devialet Phantoms";
          card = "alsa_card.pci-0000_01_00.1";
          profile = "output:hdmi-stereo";
          sink = "alsa_output.pci-0000_01_00.1.hdmi-stereo";
          volume = "100%";
        }
      ];
    };
  };
}
