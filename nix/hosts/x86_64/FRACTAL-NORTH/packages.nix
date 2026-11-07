{ pkgs, host, ... }: {
  ##### Packages #####
  environment.systemPackages = with pkgs; [
    (google-chrome.override {
      commandLineArgs = [
        "--ozone-platform=x11"
        "--use-angle=vulkan"
        "--render-node-override=/dev/dri/by-path/pci-${host.pci.nvidiaGpu}-render"
        "--enable-features=VaapiVideoDecoder,VaapiVideoEncoder"
        "--ignore-gpu-blocklist"
        "--enable-gpu-rasterization"
      ];
    })
    distrobox
    tcpdump
    ethtool
    python3
    moonlight-qt
    pciutils
    usbutils
    lmstudio
    protonup-qt
    sbctl
    fwupd
    mesa-demos
    vulkan-tools
    vdpauinfo
    libva-utils
    nvtopPackages.full
  ];
}
