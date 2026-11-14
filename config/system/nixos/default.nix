{ ... }: {
  imports = [
    ./boot.nix
    ./disk.nix
    ./graphics.nix
    ./locale.nix
    ./networking.nix
    ./nix.nix
    ./packages.nix
    ./recovery.nix
    ./opportunistic-unlock.nix
    ./sd-card.nix
    ./sleep.nix
    ./sound.nix
    ./users.nix
    ../storage/immutability
  ];
}
