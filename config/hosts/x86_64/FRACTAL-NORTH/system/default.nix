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
      swap.size = host.disk.swapSize;
    };
    tpm.device = host.tpm.device;
  };
}
