{ config, pkgs, lib, ... }:
{  
  ##### Host Name #####
  networking.hostName = "FRACTAL-NORTH";
  # Disk information
  variables.disk.device = "/dev/nvme0n1"; # This line must exist, but feel free to change the location
  variables.disk.swapSize = "65G";
  ##########################
  # Basic system settings  #
  ##########################
  system.stateVersion = "24.11"; # Match your NixOS release

  ###############
  # Bootloader  #
  ###############
  boot = {
    # Force AMD Display Core, set a 4k@120 mode if recognized
    kernelParams = [
      "amdgpu.dc=1"
      "video=HDMI-A-0:3840x2160@120"
    ];

    # Load these modules early (initrd) so the console can try high-res
    initrd.availableKernelModules = [
      "amdgpu"
      "nvme"
      "xhci_pci"
      "ahci"
      "usbhid"
      "thunderbolt"
    ];
    initrd.kernelModules = [ ];
    kernelModules = [ "kvm-amd" ];
  };

  ##########################
  # AMD microcode & GPU    #
  ##########################
  hardware.cpu.amd.updateMicrocode = true;

  # This option loads amdgpu in initrd stage (improves early console).
  # It's available in 24.11; see `nixos-option hardware.amdgpu.initrd.enable`.
  hardware.amdgpu.initrd.enable = true;

  # Provide AMD’s user-space Vulkan/OpenCL stacks
  # so that you have all GPU drivers ready in userland.
  hardware.opengl = {
    enable = true;
    driSupport32Bit = true;
    extraPackages = with pkgs; [
      amdvlk
    ];
  };

  ##########################
  # Display & Desktop      #
  ##########################
  # We enable SDDM (with Wayland) and Plasma 6.
  services.xserver = {
    enable = true;
    videoDrivers = [ "amdgpu" ];

    # SDDM display manager
    displayManager.sddm.enable = true;
    displayManager.sddm.wayland.enable = true;
    displayManager.defaultSession = "plasma";

    # Plasma 6 desktop environment
    desktopManager.plasma6.enable = true;
  };

  ##########################
  # System Packages        #
  ##########################
  environment.systemPackages = with pkgs; [
    # Basic GPU info & monitoring
    glxinfo
    radeontop
    # Vulkan driver, etc. included above in hardware.opengl.extraPackages
  ];

  ################################
  # (Optional) Networking, SSH...#
  ################################
  # networking.useDHCP = true;
  # services.openssh.enable = true;

}

