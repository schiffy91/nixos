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
    utils.drive.push = true;  # ~/Drive is the Drive client's; changes on the Drive are announced, not waited for
    dev.lmstudio.enable = true;
    dev.antigravity.enable = true;  # the Antigravity app and the agy CLI
    utils.chrome.flags = [
      "--enable-features=VaapiOnNvidiaGPUs,WaylandWindowDecorations"  # NVDEC decode; restates the wrapper's feature
      "--disable-features=WaylandWpColorManagerV1"  # Chrome blows out HDR video here
    ];
    gaming.steam.extraEnv = graphics.environment;
    gaming.sunshine = {
      capture = "kwin";
      cuda.enable = true;
    };
    # RTX 4090: 950 mV crashed under Superposition and gpu-burn; 985 and 965 mV passed.
    utils.gpuTuning = {
      enable = true;
      mode = "tuned";  # tray: Tuning > GPU > Stock | Tuned
      voltage = 965;  # mV; curve points at and above it run at `clock`
      clock = 2750;  # MHz; settles near 2730 as the stock curve shifts with temperature
      memoryOffset = 1500;  # MSI Afterburner MHz (11751 MHz in nvidia-smi)
      powerLimit = 75;  # percent of 450 W
    };
    utils.cpuTuning = {
      enable = true;  # tray: Tuning > CPU > Stock | Tuned (BIOS) | Tuned (Software)
      mode = "bios";  # undervolt set in bios
      # Linux core N = CPUs N and N+16. BIOS Curve Optimizer lists CCD0 Core 0-7 and
      # CCD1 Core 0-7, so Linux cores 8-15 are BIOS CCD1 Core 0-7. Negative counts.
      offsets = [
        (-10)  # core 0: CPUs 0,16  = BIOS CCD0 Core 0
        (-6)   # core 1: CPUs 1,17  = BIOS CCD0 Core 1
        (-11)  # core 2: CPUs 2,18  = BIOS CCD0 Core 2
        (-11)  # core 3: CPUs 3,19  = BIOS CCD0 Core 3
        (-5)   # core 4: CPUs 4,20  = BIOS CCD0 Core 4
        (-11)  # core 5: CPUs 5,21  = BIOS CCD0 Core 5
        (-11)  # core 6: CPUs 6,22  = BIOS CCD0 Core 6
        (-11)  # core 7: CPUs 7,23  = BIOS CCD0 Core 7
        (-10)  # core 8: CPUs 8,24  = BIOS CCD1 Core 0
        (-11)  # core 9: CPUs 9,25  = BIOS CCD1 Core 1
        (-11)  # core 10: CPUs 10,26 = BIOS CCD1 Core 2
        (-11)  # core 11: CPUs 11,27 = BIOS CCD1 Core 3
        (-11)  # core 12: CPUs 12,28 = BIOS CCD1 Core 4
        (-11)  # core 13: CPUs 13,29 = BIOS CCD1 Core 5
        (-11)  # core 14: CPUs 14,30 = BIOS CCD1 Core 6
        (-11)  # core 15: CPUs 15,31 = BIOS CCD1 Core 7
      ];
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
