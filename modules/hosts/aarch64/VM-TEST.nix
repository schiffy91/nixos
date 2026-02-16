{ lib, ... }: {
  networking.hostName = "VM-TEST";
  users.users.root.openssh.authorizedKeys.keys = [
    "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAINJsoluI1m5T4iwuCbpSdHvLVdemN3v7wMrqk4e+XJA0 alexanderschiffhauer@mbp21.local"
  ];
  boot.kernelParams = [ "console=ttyS0,115200n8" ];
  boot.initrd.availableKernelModules = [ "virtio_pci" "virtio_blk" "virtio_net" ];
  services.openssh.settings.UseDns = lib.mkForce false;
  settings.networking.lanSubnet = "10.0.2.0/24";
  settings.disk.device = "/dev/vda";
  settings.disk.encryption.enable = false;
  settings.disk.immutability.enable = false;
  settings.disk.swap.size = "2G";
  settings.desktop.environment = "none";
}
