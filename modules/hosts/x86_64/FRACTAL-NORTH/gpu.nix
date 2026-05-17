{ config, pkgs, host, ... }: {
  # KWIN_DRM_DEVICES splits on ':' with no escape — by-path symlinks contain colons.
  services.udev.extraRules = ''
    SUBSYSTEM=="drm", KERNEL=="card[0-9]*", ENV{ID_PATH}=="pci-${host.pci.nvidiaGpu}", SYMLINK+="dri/nvidia-card"
    SUBSYSTEM=="drm", KERNEL=="card[0-9]*", ENV{ID_PATH}=="pci-${host.pci.amdGpu}", SYMLINK+="dri/amd-card"
  '';
  boot = {
    kernelParams = [ "amdgpu.dc=1" ];
    consoleLogLevel = 0;  # silences MSFT8000 i2c spam
    kernelModules = [ "i2c-dev" ];
  };
  hardware = {
    amdgpu.initrd.enable = true;
    i2c.enable = true;
    nvidia = {
      open = true;
      modesetting.enable = true;
      package = config.boot.kernelPackages.nvidiaPackages.latest;
      nvidiaSettings = true;
      powerManagement.enable = true;
      prime = {
        offload.enable = true;
        offload.enableOffloadCmd = true;
        amdgpuBusId = host.pci.amdPrimeBus;
        nvidiaBusId = host.pci.nvidiaPrimeBus;
      };
    };
    graphics = {
      enable32Bit = true;
      extraPackages = with pkgs; [
        nvidia-vaapi-driver
        libva-vdpau-driver
        libvdpau-va-gl
        vulkan-loader
        vulkan-validation-layers
        vulkan-tools
        vulkan-hdr-layer-kwin6  # ENABLE_HDR_WSI=1 → Vulkan HDR via Wayland color-management
      ];
      extraPackages32 = with pkgs.pkgsi686Linux; [
        nvidia-vaapi-driver
        libva-vdpau-driver
        libvdpau-va-gl
      ];
    };
  };
  services.xserver.videoDrivers = [ "amdgpu" "nvidia" ];  # AMD scans out, NVIDIA renders via PRIME
  environment.sessionVariables = host.nvidiaOffloadEnv // {
    KWIN_DRM_DEVICES = host.display.driCards;
    KWIN_DRM_ALLOW_NVIDIA_COLORSPACE = "1";  # required for HDR on NVIDIA-attached outputs (DP-3 streaming display)
  };
  ##### GPU LED Off (OpenRGB) #####
  services.hardware.openrgb.enable = true;
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
}
