{ ... }:
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
    externalNvmeId = "nvme-WD_BLACK_SN850X_4000GB_22461L800626";
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
  display = {
    primaryOutput = "DP-1";
    amdCard    = "/dev/dri/amd-card";     # udev symlink, see gpu.nix
    nvidiaCard = "/dev/dri/nvidia-card";  # udev symlink, see gpu.nix
    driCards   = "/dev/dri/nvidia-card:/dev/dri/amd-card";
  };
  network = {
    primaryInterface = "eno2";  # Stable onboard NIC; TB ethernet (eth0) is deprioritized
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
  imports = [
    ./audio.nix
    ./gpu.nix
    ./input.nix
    ./networking.nix
    ./packages.nix
    ./steam.nix
    ./sunshine.nix
    ./vfio.nix
    ./virtualization.nix
  ];
  _module.args.host = {
    inherit pci disk input display network audio nvidiaOffloadEnv;
  };
  networking.hostName = "FRACTAL-NORTH";
  settings = {
    disk = {
      device = "/dev/nvme0n1";
      encryption.enable = true;
      immutability.enable = true;
      swap.size = "65G";
    };
    tpm.device = "/dev/tpmrm0";
    desktop = {
      environment = "plasma-wayland";
      scalingFactor = 2.5;
      primaryOutput = display.primaryOutput;
    };
  };
}
