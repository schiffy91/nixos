{ config, pkgs, ... }:
{
  ##### Host Name #####
  networking.hostName = "FRACTAL-NORTH";

  # Basic display configuration
  services.xserver = {
    enable = true;
    videoDrivers = [ "amdgpu" ];
  };

  # AMD GPU support
  hardware.opengl = {
    enable = true;
    driSupport = true;
    driSupport32Bit = true;
    extraPackages = with pkgs; [
      amdvlk
      rocm-opencl-icd
      rocm-opencl-runtime
    ];
  };

  # Essential system packages
  environment.systemPackages = with pkgs; [
    pciutils    # For hardware inspection
    glxinfo     # For GPU info
    radeontop   # For AMD GPU monitoring
  ];
}