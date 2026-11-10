_:
let
  gpus = {
    nvidia     = { driver = "nvidia"; pci = "0000:01:00.0"; audioPci = "0000:01:00.1"; connectors = { DP = 3; HDMI-A = 1; }; };
    integrated = { driver = "amdgpu"; pci = "0000:6b:00.0"; connectors = { DP = 2; HDMI-A = 1; }; enable = false; };  # off in firmware so the 4090 shows the boot menu
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
    primary.scaleFactor = 2.5;
  };
  network = {
    primaryInterface = "eno2";  # onboard NIC; TB ethernet (eth0) deprioritized
    lanSubnet        = "192.168.50.0/24";
    unmanagedMacs    = [ "24:f5:a2:f1:4d:9b" ];  # Linksys USB3GIGV1 dongle (breaks WoL routing)
  };
  audio = {
    quadCortexName             = "Quad Cortex";
    quadCortexInputPattern     = "~alsa_input.*Neural_DSP_Quad_Cortex.*";
    quadCortexOutputPattern    = "~alsa_output.*Neural_DSP_Quad_Cortex.*";
    samsungTvName             = "Samsung TV";
    samsungTvAlsaName         = "QCQ90S";
    samsungTvOutputPattern    = "~alsa_output.pci-${builtins.replaceStrings [ ":" ] [ "_" ] gpus.nvidia.audioPci}.*";
    logitechCameraInputPattern = "~alsa_input.*Logi_4K_Pro.*";
  };
in {
  imports = [ ./system ./pkgs ];
  _module.args.host = {
    inherit gpus disk tpm input display network audio;
  };
  networking.hostName = "FRACTAL-NORTH";
}
