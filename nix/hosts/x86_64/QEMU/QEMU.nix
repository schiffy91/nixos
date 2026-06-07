{ lib, ... }:
{
  imports = [
    ./guest.nix
    ./uefi.nix
  ];

  networking.hostName = "QEMU";

  settings = {
    apps.enable = false;
    boot.timeout = 0;
    desktop.enable = false;
    disk = {
      device = "/dev/vda";
      encryption.enable = false;
      immutability = {
        enable = true;
        mode = "reset";
      };
      swap.size = "4G";
    };
    networking = {
      identityAgent = "none";
      lanSubnet = "10.0.2.0/24";
    };
    users.admin = {
      autoLogin.enable = false;
      autoLock.enable = false;
      autoUnlockWallet.enabled = false;
      homeManager.enable = false;
    };
    users.agent.enable = false;
  };

  security.sudo.wheelNeedsPassword = lib.mkForce false;
}
