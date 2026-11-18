{ host, lib, ... }: {
  imports = [ ./audio.nix ./cpu.nix ./display.nix ./gpu.nix ./input.nix ./networking.nix ];
  boot.lanzaboote.settings.console-mode = lib.mkForce "3";  # sharpest on the XDR; the systemd-boot option rejects 3
  settings = {
    boot.timeout = 1;
    disk = {
      inherit (host.disk) device;
      encryption.enable = true;
      encryption.opportunisticUnlock.enable = true;
      recovery.enable = true;
      immutability.enable = true;
      immutability.base = "empty";  # every boot starts from a new root; CLEAN is no longer read
      swap.size = host.disk.swapSize;
    };
    tpm.device = host.tpm.device;
    nix.minFreeGiB = 200;  # of 7.3 TB
    nix.maxFreeGiB = 500;
  };
}
