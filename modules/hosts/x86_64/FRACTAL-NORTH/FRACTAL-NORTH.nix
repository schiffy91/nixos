{ ... }:
let
  pci = {
    nvidiaGpu      = "0000:01:00.0";
    nvidiaAudio    = "0000:01:00.1";
    amdGpu         = "0000:6b:00.0";
    nvidiaPrimeBus = "PCI:1:0:0";
    amdPrimeBus    = "PCI:107:0:0";
  };
  disk = {
    device         = "/dev/nvme0n1";
    swapSize       = "65G";
    externalNvmeId = "nvme-WD_BLACK_SN850X_4000GB_22461L800626";
  };
  tpm = {
    device = "/dev/tpmrm0";
  };
  input = {
    mouse = {
      vendorId  = "1133";    # 0x046D Logitech
      productId = "50504";   # 0xC548 Bolt receiver
      name      = "Logitech USB Receiver Mouse";
    };
  };
  display = {
    primaryName = "DP-1";
    scaleFactor = 2.5;
    driCards    = "/dev/dri/nvidia-card:/dev/dri/amd-card";  # KWIN_DRM_DEVICES; symlinks from gpu.nix udev
  };
  network = {
    primaryInterface = "eno2";  # onboard NIC; TB ethernet (eth0) deprioritized
    lanSubnet        = "192.168.50.0/24";
    unmanagedMacs    = [ "24:f5:a2:f1:4d:9b" ];  # Linksys USB3GIGV1 dongle (breaks WoL routing)
  };
  audio = {
    quadCortexInputPattern     = "~alsa_input.*Neural_DSP_Quad_Cortex.*";
    quadCortexOutputPattern    = "~alsa_output.*Neural_DSP_Quad_Cortex.*";
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
    ./cpu.nix
    ./gpu.nix
    ./input.nix
    ./networking.nix
    ./packages.nix
    ./steam.nix
    ./sunshine.nix
  ];
  _module.args.host = {
    inherit pci disk tpm input display network audio nvidiaOffloadEnv;
  };
  networking.hostName = "FRACTAL-NORTH";
  settings = {
    disk = {
      device = disk.device;
      encryption.enable = true;
      immutability.enable = true;
      swap.size = disk.swapSize;
    };
    tpm.device = tpm.device;
    desktop.outputs = [{
      name        = display.primaryName;
      scaleFactor = display.scaleFactor;
      primary     = true;
    }];
  };
}
