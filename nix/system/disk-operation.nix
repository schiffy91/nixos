{ config, lib, ... }:

{
  imports = [ ./disk.nix ];

  # Disk-Operation is a disko entrypoint, not an installed boot target. These
  # defaults keep the module graph evaluable when `nix flake check` validates
  # every nixosConfiguration.
  boot.loader.grub.enable = lib.mkForce false;
  boot.loader.systemd-boot.enable = lib.mkForce false;
  boot.loader.generic-extlinux-compatible.enable = lib.mkForce false;

  users.users.${config.settings.users.admin.username} = {
    isNormalUser = lib.mkDefault true;
  };
}
