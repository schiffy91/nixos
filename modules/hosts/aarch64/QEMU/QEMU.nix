{ lib, ... }:
{
  imports = [
    ./guest.nix
    ./uefi.nix
  ];

  networking.hostName = "QEMU";

  settings = {
    boot.timeout = 0;
    disk = {
      device = "/dev/vda";
      encryption.enable = false;
      immutability = {
        enable = true;
        implementation = "semipermeable_membrane";
        semipermeable_membrane.enable = true;
        semipermeable_membrane.mode = "reset";
      };
      swap.size = "4G";
    };
    networking = {
      identityAgent = "none";
      lanSubnet = "10.0.2.0/24";
    };
    user.admin = {
      autoLogin.enable = false;
      autoLock.enable = false;
      autoUnlockWallet.enabled = false;
      homeManager.enable = false;
    };
  };

  security.sudo.wheelNeedsPassword = lib.mkForce false;
}
