{ config, pkgs, host, ... }: {
  settings.graphics = {
    inherit (host) gpus;
    display = "nvidia";
  };
  boot = {
    consoleLogLevel = 0;  # silences MSFT8000 i2c spam
    kernelModules = [ "i2c-dev" ];
  };
  hardware = {
    i2c.enable = true;
    nvidia = {
      open = true;
      modesetting.enable = true;
      package = let latest = config.boot.kernelPackages.nvidiaPackages.latest; in latest // {
        open = latest.open.overrideAttrs (old: {
          patches = (old.patches or []) ++ [ ./nvidia-dsc-wide-modes-use-two-heads.patch ];  # XDR 6K needs DSC on two heads
        });
      };
      nvidiaSettings = true;
      powerManagement.enable = true;
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
      ];
      extraPackages32 = with pkgs.pkgsi686Linux; [
        nvidia-vaapi-driver
        libva-vdpau-driver
        libvdpau-va-gl
      ];
    };
  };
}
