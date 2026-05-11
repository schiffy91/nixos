{ ... }: {
  boot.kernelModules = [ "kvm-amd" ];
  ##### Virtualization #####
  services.qemuGuest.enable = true;
  programs.virt-manager.enable = true;
  virtualisation = {
    libvirtd = {
      enable = true;
      qemu.swtpm.enable = true;
    };
    spiceUSBRedirection.enable = true;
    podman = {
      enable = true;
      dockerCompat = true;
    };
  };
}
