{ config, pkgs, lib, ... }: {
  ##### Host Name #####
  networking.hostName = "FRACTAL-NORTH";
  ##### Disk Information #####
  settings.disk.device = "/dev/nvme0n1";
  settings.disk.encryption.enable = true;
  settings.disk.immutability.enable = true;
  settings.disk.swap.size = "65G";
  ##### TPM2 #####
  settings.tpm.device = "/dev/tpmrm0";
  ##### Display #####
  settings.desktop.scalingFactor = 2.5;
  ##### AMD #####
  boot.kernelParams = [
    "amdgpu.dc=1"                   # AMD GPU
    "amd_iommu=on"                  # Enable IOMMU for VFIO passthrough
    "iommu=pt"                      # IOMMU passthrough mode (better performance)
  #  "video=HDMI-A-0:3840x2160@120"  # Force 4k120
  ];
  boot.kernelModules = [ "kvm-amd" "vfio" "vfio_pci" "vfio_iommu_type1" ];
  hardware.firmware = [ pkgs.linux-firmware ];
  hardware.cpu.amd.updateMicrocode = true;
  hardware.amdgpu.initrd.enable = true;
  ##### Desktop Environment #####
  settings.desktop.environment = "plasma-wayland";
  ##### NVIDIA #####
  services.xserver.videoDrivers = [ "nvidia" "amdgpu" ]; # NVIDIA primary, AMD as display sink
  hardware.nvidia = {
    open = false;
    modesetting.enable = true;
    package = config.boot.kernelPackages.nvidiaPackages.latest;
    nvidiaSettings = true;
    powerManagement.enable = true;
    prime = {
      sync.enable = true;               # NVIDIA renders everything, AMD iGPU is display output
      amdgpuBusId = "PCI:107:0:0";      # AMD iGPU at 6b:00.0 (hex 6b = 107 dec) - display via TB4
      nvidiaBusId = "PCI:1:0:0";        # RTX 4090 at 01:00.0 - all rendering
    };
  };
  hardware.graphics.extraPackages = with pkgs; [
    nvidia-vaapi-driver
    vulkan-loader
    vulkan-validation-layers
  ];
  hardware.graphics.enable32Bit = true;
  environment.sessionVariables = {
    __GLX_VENDOR_LIBRARY_NAME = "nvidia";
    GBM_BACKEND = "nvidia-drm";
    LIBVA_DRIVER_NAME = "nvidia";
    VDPAU_DRIVER = "nvidia";
  };
  ##### Virtualization #####
  services.qemuGuest.enable = true;
  programs.virt-manager.enable = true;
  virtualisation = {
    libvirtd = {
      enable = true;
      qemu = {
        package = pkgs.qemu;
        swtpm.enable = true;
        runAsRoot = false;
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
    mpv
    lmstudio
    sbctl
    fwupd
    nixd
    # GPU diagnostic tools
    mesa-demos       # provides glxinfo
    vulkan-tools   # provides vulkaninfo
    vdpauinfo          # VDPAU info
    libva-utils     # vainfo for VA-API
    nvtopPackages.full      # GPU monitoring (supports NVIDIA + AMD)
  ];
  ##### Networking #####
  settings.networking.lanSubnet = "10.0.0.0/24";
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
    extraPackages = with pkgs; [ kdePackages.breeze ];
    package = pkgs.steam.override {
      extraLibraries = pkgs': with pkgs'; [ pipewire.jack ];
    };
  };
  settings.networking.ports.tcp = [ 47984 47989 47990 48010 ];
  settings.networking.ports.udp = (lib.range 47998 48000) ++ (lib.range 8000 8010);
  ##### Thunderbolt #####
  services.hardware.bolt.enable = true;
  ##### Rocksmith / Quad Cortex #####
  services.pipewire.extraConfig.pipewire."10-low-latency" = {
    "context.properties" = {
      "default.clock.min-quantum" = 32;
      "default.clock.rate" = 48000;
      "default.clock.allowed-rates" = [ 48000 ];
    };
  };
  users.users.${config.settings.user.admin.username}.extraGroups = [ "audio" "rtkit" "pipewire" ];
  security.pam.loginLimits = [
    { domain = "@audio"; item = "memlock"; type = "-"; value = "unlimited"; }
    { domain = "@audio"; item = "rtprio"; type = "-"; value = "99"; }
  ];
  services.pipewire.wireplumber.extraConfig."51-quad-cortex"."monitor.alsa.rules" = [
    {
      matches = [{ "node.name" = "~alsa_input.*Neural_DSP_Quad_Cortex.*"; }];
      actions.update-props = {
        "session.suspend-timeout-seconds" = 0;
        "priority.session" = 2500;
        "priority.driver" = 2500;
      };
    }
    {
      matches = [{ "node.name" = "~alsa_output.*Neural_DSP_Quad_Cortex.*"; }];
      actions.update-props."session.suspend-timeout-seconds" = 0;
    }
    {
      matches = [{ "node.name" = "~alsa_input.*Logi_4K_Pro.*"; }];
      actions.update-props."node.force-quantum" = 1024;
    }
  ];
}
