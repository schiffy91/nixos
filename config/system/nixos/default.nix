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
    ./storage.nix
    ./sleep.nix
    ./sound.nix
    ./home-folders.nix
    ./users.nix
    ../storage/immutability
  ];
}
