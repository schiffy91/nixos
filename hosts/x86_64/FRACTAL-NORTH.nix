{ config, pkgs, ... }:
{  
  ##### Host Name #####
  networking.hostName = "FRACTAL-NORTH";
  # Disk information
  variables.disk.device = "/dev/nvme0n1"; # This line must exist, but feel free to change the location
  variables.disk.swapSize = "65G";
  # Dual GPU drivers
  services.xserver.videoDrivers = [ "nvidia" "amdgpu" ];
  # AMD drivers
  boot.kernelParams = [
    "amdgpu.dc=1"
    "video=HDMI-A-0:3840x2160@120"
  ];
  boot.kernelModules = [ "kvm-amd" ];
  hardware.firmware = [
    pkgs.linux-firmware
  ];
  hardware.cpu.amd.updateMicrocode = true;
  hardware.amdgpu.initrd.enable = true;
  hardware.nvidia = {
    open = false;
    package = config.boot.kernelPackages.nvidiaPackages.stable;
    nvidiaSettings = true;
    powerManagement.enable = true;
    prime = {
      offload = {
        enable = true;
        enableOffloadCmd = true;
      };
      amdgpuBusId = "PCI:1:0:0";
      nvidiaBusId = "PCI:69:0:0";
    };
  };
  # Virtualization (libvirt, podman)
  services.qemuGuest.enable = true;
  programs.virt-manager.enable = true;
  virtualisation = {
    libvirtd = {
      enable = true;
      qemu = {
        package = pkgs.qemu;
        ovmf.enable = true;
        ovmf.packages = [ pkgs.OVMFFull.fd ];
        swtpm.enable = true;
        runAsRoot = false;
      };
    };
    spiceUSBRedirection.enable = true;
    podman = {
      enable = true;
      dockerCompat = true;
    };
  };

  # Packages
  environment.systemPackages = with pkgs; [
    google-chrome
    distrobox
    vscode
    pciutils
    usbutils
    virt-manager
    virt-viewer
    spice 
    spice-gtk
    spice-protocol
    win-virtio
    win-spice
  ];

  # Moonlight
  services.sunshine = {
    enable = true;
    openFirewall = false;
    autoStart = true;
    capSysAdmin = true;
  };
}
