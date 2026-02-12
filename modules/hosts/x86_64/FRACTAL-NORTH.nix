{ config, pkgs, lib, ... }: {
  ##### Host Name #####
  networking.hostName = "FRACTAL-NORTH";
  ##### Disk Information #####
  settings.disk.device = "/dev/nvme0n1";
  settings.disk.encryption.enable = true;
  settings.disk.immutability.enable = true;
  settings.disk.swap.size = "65G";
  ##### Drivers #####
  services.xserver.videoDrivers = [ "amdgpu" "nvidia" ]; # AMD first (primary for display), NVIDIA second (for rendering via PRIME)
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
  # Note: hardware.graphics.enable and enable32Bit are set in desktop.nix
  hardware.graphics.extraPackages = with pkgs; [
    nvidia-vaapi-driver  # NVIDIA hardware video decode
  ];
  ##### NVIDIA #####
  hardware.nvidia = {
    open = true;
    modesetting.enable = true;
    package = config.boot.kernelPackages.nvidiaPackages.latest;
    nvidiaSettings = true;
    powerManagement.enable = true;
    prime = {
      reverseSync.enable = true;    # Reverse Sync: NVIDIA renders, AMD iGPU outputs to display
      amdgpuBusId = "PCI:107:0:0";  # AMD iGPU at 6b:00.0 (hex 6b = 107 dec)
      nvidiaBusId = "PCI:1:0:0";    # RTX 4090 at 01:00.0
    };
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
  ##### Desktop Environment #####
  settings.desktop.environment = "plasma-wayland";

  ##### Environment Variables for Hardware Acceleration #####
  environment.sessionVariables = {
    # Force Chrome to use NVIDIA GPU for rendering
    __NV_PRIME_RENDER_OFFLOAD = "1";
    __GLX_VENDOR_LIBRARY_NAME = "nvidia";
    # Enable VA-API for video decode
    LIBVA_DRIVER_NAME = "nvidia";
  };
  ##### Packages #####
  environment.systemPackages = with pkgs; [
    google-chrome
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
  settings.networking.ports.tcp = [ 47984 47989 47990 48010 ];
  settings.networking.ports.udp = (lib.range 47998 48000) ++ (lib.range 8000 8010);
  ##### Thunderbolt #####
  services.hardware.bolt.enable = true;
}
