{ ... }: {
  imports = [
    ./pkgs
    ./system/nixos/boot.nix
    ./pkgs/de/kde
    ./system/nixos/disk.nix
    ./system/nixos/graphics.nix
    ./system/nixos/locale.nix
    ./system/nixos/networking.nix
    ./system/nixos/nix.nix
    ./system/nixos/packages.nix
    ./system/nixos/recovery.nix
    ./system/nixos/opportunistic-unlock.nix
    ./system/nixos/sleep.nix
    ./system/nixos/sound.nix
    ./system/nixos/users.nix
    ./system/storage/immutability
  ];
}
