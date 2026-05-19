{ pkgs, host, inputs, ... }: {
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
    (mpv.override { youtubeSupport = false; })  # avoids V8 source build chain
    lmstudio
    protonup-qt
    sbctl
    fwupd
    nixd
    claude-code
    inputs.codex.packages.${pkgs.system}.default
    (pkgs.cider-2.overrideAttrs (old: {
      postFixup = (old.postFixup or "") + ''
        substituteInPlace $out/share/applications/cider-2.desktop \
          --replace-fail "Exec=cider-2" "Exec=cider-2 --force-device-scale-factor=1 --ozone-platform=wayland --enable-features=WaylandWindowDecorations"
      '';
    }))
    solaar
    mesa-demos
    vulkan-tools
    vdpauinfo
    libva-utils
    nvtopPackages.full
  ];
}
