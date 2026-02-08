{ config, pkgs, lib, ... }: {
  ##### Host Name #####
  networking.hostName = "FRACTAL-NORTH";
  ##### Disk Information #####
  settings.disk.device = "/dev/nvme0n1";
  settings.disk.encryption.enable = true;
  settings.disk.immutability.enable = true;
  settings.disk.swap.size = "65G";
  ##### Drivers #####
  services.xserver.videoDrivers = [ "nvidia" "amdgpu" ];
  ##### TPM2 #####
  settings.tpm.device = "/dev/tpmrm0";
  ##### Display #####
  settings.desktop.scalingFactor = 2.5;
  ##### AMD #####
  boot.kernelParams = [
    "amdgpu.dc=1"                   # AMD GPU
    "video=HDMI-A-0:3840x2160@120"  # Force 4k120
  ];
  boot.kernelModules = [ "kvm-amd" ];
  hardware.firmware = [ pkgs.linux-firmware ];
  hardware.cpu.amd.updateMicrocode = true;
  hardware.amdgpu.initrd.enable = true;
  ##### NVIDIA #####
  hardware.nvidia = {
    open = true;
    package = config.boot.kernelPackages.nvidiaPackages.latest;
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
  ##### Desktop Environment #####
  settings.desktop.environment = "plasma-wayland";
  ##### Packages #####
  environment.systemPackages = with pkgs; [
    google-chrome
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
    mpv
    #ollama-cuda
    fwupd
  ];
  ##### Networking #####
  settings.networking.lanSubnet = "10.0.0.0/24";
  programs.openvpn3.enable = true;
  services.resolved.enable = true;
  services.mullvad-vpn = {
    enable = true;
    package = pkgs.mullvad-vpn;
  };
  ##### Moonlight #####
  services.sunshine = {
    enable = true;
    openFirewall = false;
    autoStart = true;
    capSysAdmin = true;
  };
  settings.networking.ports.tcp = [ 47984 47989 47990 48010 ];
  settings.networking.ports.udp = (lib.range 47998 48000) ++ (lib.range 8000 8010);
  ##### Thunderbolt #####
  services.hardware.bolt.enable = true;
}
