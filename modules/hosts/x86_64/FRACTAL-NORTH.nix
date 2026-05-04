{ config, pkgs, lib, ... }:
let
  pci = {
    nvidiaGpu       = "0000:01:00.0";
    nvidiaAudio     = "0000:01:00.1";
    amdGpu          = "0000:6b:00.0";
    nvidiaPrimeBus  = "PCI:1:0:0";
    amdPrimeBus     = "PCI:107:0:0";
    nvidiaGpuId     = "10de:2684";   # RTX 4090 (AD102)
    nvidiaAudioId   = "10de:22ba";   # AD102 HDMI audio
  };
  disk = {
    nvmeId = "nvme-WD_BLACK_SN850X_4000GB_22461L800626";
  };
  input = {
    razerKeyboardEvent = "usb-Razer_Razer_BlackWidow_Lite-event-kbd";
    logitechMouseEvent = "usb-Logitech_USB_Receiver-if01-event-mouse";
    logitechBolt = {
      vendorId   = "1133";   # 0x046D
      productId  = "50504";  # 0xC548
      mouseName  = "Logitech USB Receiver Mouse";
    };
  };
  display = rec {
    primaryOutput = "DP-1";  # XDR via TB4; AMD iGPU connector name
    # KWIN_DRM_DEVICES uses ':' as path separator — can't embed PCI addresses (which contain ':').
    # Card numbers are assigned by DRM probe order: NVIDIA (PCI 01:00.0) probes first → card0, AMD iGPU (PCI 6b:00.0) → card1.
    nvidiaCard = "/dev/dri/card0";
    amdCard    = "/dev/dri/card1";
    # NVIDIA first → KWin renders on dGPU; AMD second → scans out (it owns the TB4 display).
    driCards   = "${nvidiaCard}:${amdCard}";
  };
  network = {
    primaryInterface = "eno2";  # Stable onboard NIC; TB ethernet (eth0) is optional + goes away during sleep
    lanSubnet        = "192.168.50.0/24";
  };
  audio = {
    quadCortexInputPattern  = "~alsa_input.*Neural_DSP_Quad_Cortex.*";
    quadCortexOutputPattern = "~alsa_output.*Neural_DSP_Quad_Cortex.*";
    logitechCameraInputPattern = "~alsa_input.*Logi_4K_Pro.*";
  };
  nvidiaOffloadEnv = {
    __NV_PRIME_RENDER_OFFLOAD          = "1";
    __NV_PRIME_RENDER_OFFLOAD_PROVIDER = "NVIDIA-G0";
    __VK_LAYER_NV_optimus              = "NVIDIA_only";
    __GLX_VENDOR_LIBRARY_NAME          = "nvidia";
    LIBVA_DRIVER_NAME                  = "nvidia";
    VDPAU_DRIVER                       = "nvidia";
  };
in {
  ##### Host Name #####
  networking.hostName = "FRACTAL-NORTH";
  ##### XBox Controller #####
  hardware.xone.enable = true;
  ##### Logitech Bolt receiver (mice/keyboards) #####
  hardware.logitech.wireless.enable = true;
  home-manager.users.${config.settings.user.admin.username}.programs.plasma.configFile.kcminputrc."Libinput][${input.logitechBolt.vendorId}][${input.logitechBolt.productId}][${input.logitechBolt.mouseName}".PointerAccelerationProfile = 1;
  ##### Flatpak #####
  nixpkgs.overlays = [ (final: prev: { openldap = prev.openldap.overrideAttrs { doCheck = false; }; }) ];
  services.flatpak = {
    enable = true;
    remotes = [{ name = "flathub"; location = "https://flathub.org/repo/flathub.flatpakrepo"; }];
    packages = [ "com.usebottles.bottles" "org.freedesktop.Platform.VulkanLayer.gamescope//25.08" ];
    update.onActivation = true;
  };
  ##### Disk Information #####
  settings.disk.device = "/dev/nvme0n1";
  settings.disk.encryption.enable = true;
  settings.disk.immutability.enable = true;
  settings.disk.swap.size = "65G";
  ##### TPM2 #####
  settings.tpm.device = "/dev/tpmrm0";
  ##### Display #####
  settings.desktop.scalingFactor = 2.5;
  settings.desktop.primaryOutput = display.primaryOutput;
  ##### AMD #####
  boot.kernelParams = [
    "amdgpu.dc=1"                   # AMD GPU
    "amd_iommu=on"                  # Enable IOMMU for VFIO passthrough
    "iommu=pt"                      # IOMMU passthrough mode (better performance)
  ];
  boot.consoleLogLevel = 0;         # Suppress kernel messages during boot (MSFT8000 i2c noise)
  boot.kernelModules = [ "kvm-amd" "vfio" "vfio_pci" "vfio_iommu_type1" "i2c-dev" ];
  boot.blacklistedKernelModules = [ "hid_sensor_hub" ];
  hardware.firmware = [ pkgs.linux-firmware ];
  hardware.cpu.amd.updateMicrocode = true;
  hardware.amdgpu.initrd.enable = true;
  ##### Desktop Environment #####
  settings.desktop.environment = "plasma-wayland";
  ##### NVIDIA #####
  services.xserver.videoDrivers = [ "amdgpu" "nvidia" ]; # AMD first (primary for display), NVIDIA second (for rendering via PRIME)
  hardware.nvidia = {
    open = true;
    modesetting.enable = true;
    package = config.boot.kernelPackages.nvidiaPackages.latest;
    nvidiaSettings = true;
    powerManagement.enable = true;
    prime = {
      offload.enable = true;            # AMD composes desktop, NVIDIA for app offload
      offload.enableOffloadCmd = true;  # Provides nvidia-offload wrapper
      amdgpuBusId = pci.amdPrimeBus;    # AMD iGPU at 6b:00.0 (hex 6b = 107 dec) - display via TB4
      nvidiaBusId = pci.nvidiaPrimeBus; # RTX 4090 at 01:00.0 - app rendering
    };
  };
  hardware.graphics.extraPackages = with pkgs; [
    nvidia-vaapi-driver
    libva-vdpau-driver
    libvdpau-va-gl
    vulkan-loader
    vulkan-validation-layers
    vulkan-tools
  ];
  hardware.graphics.enable32Bit = true;
  hardware.graphics.extraPackages32 = with pkgs.pkgsi686Linux; [
    nvidia-vaapi-driver
    libva-vdpau-driver
    libvdpau-va-gl
  ];
  environment.sessionVariables = nvidiaOffloadEnv // {
    KWIN_DRM_DEVICES = display.driCards;
  };
  ##### GPU LED Off (OpenRGB) #####
  services.hardware.openrgb.enable = true;
  hardware.i2c.enable = true;
  systemd.services.gpu-led-off = {
    description = "Turn off NVIDIA 4090 FE LED";
    after = [ "openrgb.service" "display-manager.service" ];
    wantedBy = [ "multi-user.target" ];
    serviceConfig = {
      Type = "oneshot";
      ExecStartPre = "${pkgs.coreutils}/bin/sleep 5";
      ExecStart = "${pkgs.bash}/bin/bash -c '${pkgs.openrgb-with-all-plugins}/bin/openrgb --noautoconnect -d 1 -m Off || true'";
    };
  };
  ##### Virtualization #####
  services.qemuGuest.enable = true;
  programs.virt-manager.enable = true;
  virtualisation = {
    libvirtd = {
      enable = true;
      qemu = {
        swtpm.enable = true;
      };
    };
    spiceUSBRedirection.enable = true;
    podman = {
      enable = true;
      dockerCompat = true;
    };
  };
  ##### Packages #####
  environment.systemPackages = with pkgs; [
    (google-chrome.override {
      commandLineArgs = [
        "--ozone-platform=x11"                                                  # Use X11 backend (Wayland has DMA-BUF import issues with NVIDIA)
        "--use-angle=vulkan"                                                    # Use Vulkan through ANGLE
        "--render-node-override=/dev/dri/by-path/pci-${pci.nvidiaGpu}-render"   # Force NVIDIA GPU
        "--enable-features=VaapiVideoDecoder,VaapiVideoEncoder"
        "--ignore-gpu-blocklist"
        "--enable-gpu-rasterization"
      ];
    })
    distrobox
    pciutils
    usbutils
    looking-glass-client
    virt-manager
    virt-viewer
    spice
    spice-gtk
    spice-protocol
    virtio-win
    win-spice
    (mpv.override { youtubeSupport = false; })  # drops yt-dlp → deno → rusty-v8 → V8 source build chain on every nix flake update
    lmstudio
    protonup-qt
    sbctl
    fwupd
    nixd
    claude-code
    cider-2
    solaar
    # GPU diagnostic tools
    mesa-demos       # provides glxinfo
    vulkan-tools # provides vulkaninfo
    vdpauinfo          # VDPAU info
    libva-utils     # vainfo for VA-API
    nvtopPackages.full      # GPU monitoring (supports NVIDIA + AMD)
  ];
  ##### Networking #####
  networking.interfaces.${network.primaryInterface}.wakeOnLan.enable = true;
  settings.networking.lanSubnet = network.lanSubnet;
  settings.networking.primaryInterface = network.primaryInterface;
  programs.openvpn3.enable = true;
  services.resolved.enable = true;
  services.mullvad-vpn = {
    enable = true;
    package = pkgs.mullvad-vpn;
  };
  ##### Moonlight #####
  services.sunshine = {
    enable = true;
    openFirewall = false;
    autoStart = true;
    capSysAdmin = true;
  };
  ##### Steam #####
  programs.steam = {
    enable = true;
    package = pkgs.steam.override {
      extraLibraries = pkgs': with pkgs'; [ pipewire.jack ];
      extraEnv = nvidiaOffloadEnv;  # Steam doesn't inherit environment variables properly
    };
  };
  settings.networking.ports.tcp = [ 47984 47989 47990 48010 ];
  settings.networking.ports.udp = (lib.range 47998 48000) ++ (lib.range 8000 8010);
  ##### Thunderbolt #####
  services.hardware.bolt.enable = true;
  ##### Rocksmith / Quad Cortex #####
  services.pipewire.extraConfig.pipewire."10-low-latency" = {
    "context.properties" = {
      "default.clock.min-quantum" = config.settings.rocksmith.sampleSize;
      "default.clock.rate" = config.settings.rocksmith.sampleRate;
      "default.clock.allowed-rates" = [ config.settings.rocksmith.sampleRate ];
    };
  };
  users.users.${config.settings.user.admin.username}.extraGroups = [ "audio" "rtkit" "pipewire" ];
  security.pam.loginLimits = [
    { domain = "@audio"; item = "memlock"; type = "-"; value = "unlimited"; }
    { domain = "@audio"; item = "rtprio"; type = "-"; value = "99"; }
  ];
  services.pipewire.wireplumber.extraConfig."51-alsa-tweaks"."monitor.alsa.rules" = [
    {
      matches = [{ "node.name" = audio.quadCortexInputPattern; }];
      actions.update-props = {
        "session.suspend-timeout-seconds" = 0;
        "priority.session" = 2500;
        "priority.driver" = 2500;
      };
    }
    {
      matches = [{ "node.name" = audio.quadCortexOutputPattern; }];
      actions.update-props = {
        "session.suspend-timeout-seconds" = 0;
        "node.driver" = false;
      };
    }
    {
      matches = [{ "node.name" = audio.logitechCameraInputPattern; }];
      actions.update-props."node.disabled" = true;
    }
  ];
  ##### VFIO #####
  settings.vfio.enable = true;
  settings.vfio.vmName = "win11";
  settings.vfio.gpuPci = pci.nvidiaGpu;
  settings.vfio.audioPci = pci.nvidiaAudio;
  settings.vfio.nvmeId = disk.nvmeId;
  settings.vfio.keyboardEvent = input.razerKeyboardEvent;
  settings.vfio.mouseEvent = input.logitechMouseEvent;
  settings.vfio.lookingGlass.enable = true;
  settings.vfio.evdev.enable = true;
  # Boot entry where NVIDIA is bound to vfio-pci at boot, never loaded by Linux.
  # KDE runs on the AMD iGPU only. `virsh start win11` instantly attaches the 4090.
  # Pick "vfio" at the systemd-boot menu when you want to run the Windows VM.
  specialisation.vfio.configuration = {
    boot = {
      kernelParams = [ "vfio-pci.ids=${pci.nvidiaGpuId},${pci.nvidiaAudioId}" ];
      blacklistedKernelModules = [ "nvidia" "nvidia_drm" "nvidia_modeset" "nvidia_uvm" "nvidiafb" "nouveau" ];
      initrd.kernelModules = [ "vfio_pci" "vfio" "vfio_iommu_type1" ];
    };
    hardware.nvidia = {
      modesetting.enable = lib.mkForce false;
      powerManagement.enable = lib.mkForce false;
      prime.offload.enable = lib.mkForce false;
      prime.offload.enableOffloadCmd = lib.mkForce false;
    };
    services.xserver.videoDrivers = lib.mkForce [ "amdgpu" ];
    environment.sessionVariables = lib.mkForce {
      KWIN_DRM_DEVICES = display.amdCard;  # AMD iGPU only (NVIDIA bound to vfio-pci)
    };
    programs.steam.package = lib.mkForce (pkgs.steam.override {
      extraLibraries = pkgs': with pkgs'; [ pipewire.jack ];
    });
    systemd.services.gpu-led-off.enable = lib.mkForce false;
  };
}
