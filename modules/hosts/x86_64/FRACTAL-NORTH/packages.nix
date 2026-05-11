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
    (mpv.override { youtubeSupport = false; })  # avoids V8 source build chain
    lmstudio
    protonup-qt
    sbctl
    fwupd
    nixd
    claude-code
    cider-2
    solaar
    mesa-demos
    vulkan-tools
    vdpauinfo
    libva-utils
    nvtopPackages.full
  ];
}
