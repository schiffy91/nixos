{ pkgs, host, ... }: {
  imports = [ ./desktop.nix ];
  settings.apps = {
    dev.packages = with pkgs; [ python3 ];
    gaming.packages = with pkgs; [ protonup-qt ];
    dev.distrobox.enable = true;
    gaming.moonlight.enable = true;
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
    gaming.steam.extraEnv = host.nvidiaOffloadEnv;
    gaming.sunshine = {
      capture = "kwin";
      cuda.enable = true;
      virtualDisplay.primaryOutput = host.display.primary.name;
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
      displays = { "DP-1" = "ProXDR"; "HDMI-A-2" = "S89C"; };
      layout = [{ display = "HDMI-A-2"; position = "left-of"; relative_to = "DP-1"; }];
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
