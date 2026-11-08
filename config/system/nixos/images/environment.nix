# Shared by the recovery netboot image and the installer ISO: a rescue
# environment that boots to root, answers SSH on the admin's keys and carries
# the tools either job needs.
{ authorizedKeys, stateVersion, safeGitDirectories }:
{ lib, pkgs, ... }: {
  system.stateVersion = stateVersion;
  documentation.enable = lib.mkDefault false;
  nix.settings.experimental-features = [ "nix-command" "flakes" ];

  services.openssh = {
    enable = true;
    settings.PermitRootLogin = "prohibit-password";
  };
  services.getty.autologinUser = lib.mkForce "root";
  users.users.root.openssh.authorizedKeys.keys = authorizedKeys;

  programs.git = {
    enable = true;
    config.safe.directory = safeGitDirectories;
  };

  environment.systemPackages = with pkgs; [
    btrfs-progs
    cryptsetup
    curl
    efibootmgr
    gptfdisk
    jq
    kexec-tools
    parted
    pciutils
    rsync
    sbctl
    smartmontools
    vim
  ];
}
