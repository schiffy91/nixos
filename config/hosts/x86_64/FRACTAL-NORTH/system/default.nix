{ host, ... }: {
  imports = [ ./audio.nix ./cpu.nix ./display.nix ./gpu.nix ./input.nix ./networking.nix ];
  settings = {
    boot.timeout = 1;
    disk = {
      inherit (host.disk) device;
      encryption.enable = true;
      encryption.opportunisticUnlock.enable = true;
      recovery.enable = true;
      immutability.enable = true;
      swap.size = host.disk.swapSize;
    };
    tpm.device = host.tpm.device;
  };
}
