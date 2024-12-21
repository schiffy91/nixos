{ config, pkgs, lib, ... }:

{
  config = {
    # System information
    nixpkgs.hostPlatform.system = "x86_64-linux";
    networking.hostName = "FRACTAL-NORTH";
    shared.driveConfig.swapSize = "65G";
    
    # Secure Boot
    boot = {
      loader.systemd-boot.enable = lib.mkForce false;
      lanzaboote = {
        enable = true;
        pkiBundle = "/etc/secureboot";
      };
    };

    # Nvidia drivers
    services.xserver.videoDrivers = [ "nvidia" "amgpu" ];
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
    nixpkgs.config.allowUnfree = true;
    environment.systemPackages = with pkgs; [
      google-chrome
      distrobox
      vscode
      pciutils
      usbutils
      sbctl 
      virt-manager
      virt-viewer
      spice 
      spice-gtk
      spice-protocol
      win-virtio
      win-spice
    ];
  };
}