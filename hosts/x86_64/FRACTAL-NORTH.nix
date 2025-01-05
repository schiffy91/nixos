{ config, pkgs, lib, ... }: {
  ##### Host Name #####
  networking.hostName = "FRACTAL-NORTH";
  ##### Disk Information #####
  variables.disk.device = "/dev/nvme0n1";
  variables.disk.swapSize = "65G";
  ##### Drivers #####
  services.xserver.videoDrivers = [ "nvidia" "amdgpu" ];
  ##### AMD #####
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
  ##### NVIDIA #####
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
  ##### Virtualization #####
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
  ##### Packages #####
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
  ##### Networking #####
  variables.networking.lanSubnet = "10.0.0.0/24";
  ##### Moonlight #####
  services.sunshine = {
    enable = true;
    openFirewall = false;
    autoStart = true;
    capSysAdmin = true;
  };
  variables.networking.ports.tcp = [ 47984 47989 47990 48010 ];
  variables.networking.ports.udp = (lib.range 47998 48000) ++ (lib.range 8000 8010);}
