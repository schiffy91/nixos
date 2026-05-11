{ pkgs, lib, host, ... }: {
  boot.kernelModules = [ "vfio" "vfio_pci" "vfio_iommu_type1" ];
  ##### VFIO #####
  settings.vfio = {
    enable = true;
    vmName = "win11";
    gpuPci = host.pci.nvidiaGpu;
    audioPci = host.pci.nvidiaAudio;
    externalNvmeId = host.disk.externalNvmeId;
    keyboardEvent = host.input.razerKeyboardEvent;
    mouseEvent = host.input.logitechMouseEvent;
    lookingGlass.enable = true;
    evdev.enable = true;
  };
  specialisation.vfio.configuration = {
    boot = {
      kernelParams = [ "vfio-pci.ids=${host.pci.nvidiaGpuId},${host.pci.nvidiaAudioId}" ];
      blacklistedKernelModules = [ "nvidia" "nvidia_drm" "nvidia_modeset" "nvidia_uvm" "nvidiafb" "nouveau" ];
      initrd.kernelModules = lib.mkBefore [ "vfio_pci" "vfio" "vfio_iommu_type1" ];  # must beat amdgpu in initrd
    };
    hardware.nvidia = {
      modesetting.enable = lib.mkForce false;
      powerManagement.enable = lib.mkForce false;
      prime.offload.enable = lib.mkForce false;
      prime.offload.enableOffloadCmd = lib.mkForce false;
    };
    services.xserver.videoDrivers = lib.mkForce [ "amdgpu" ];
    environment.sessionVariables = {
      KWIN_DRM_DEVICES                   = lib.mkForce host.display.amdCard;
      __NV_PRIME_RENDER_OFFLOAD          = lib.mkForce "";
      __NV_PRIME_RENDER_OFFLOAD_PROVIDER = lib.mkForce "";
      __VK_LAYER_NV_optimus              = lib.mkForce "";
      __GLX_VENDOR_LIBRARY_NAME          = lib.mkForce "";
      LIBVA_DRIVER_NAME                  = lib.mkForce "";
      VDPAU_DRIVER                       = lib.mkForce "";
    };
    programs.steam.package = lib.mkForce (pkgs.steam.override {
      extraLibraries = pkgs': with pkgs'; [ pipewire.jack ];
    });
    systemd.services.gpu-led-off.enable = lib.mkForce false;
  };
}
