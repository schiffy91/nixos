{ system ? builtins.currentSystem }:
let
  locked = (builtins.fromJSON (builtins.readFile ../../../flake.lock)).nodes.nixpkgs.locked;
  pkgs = import (builtins.fetchTree { inherit (locked) type owner repo rev narHash; }) { inherit system; };
  inputs.btrc = let node = (builtins.fromJSON (builtins.readFile ../../../flake.lock)).nodes.btrc.locked;
    in builtins.getFlake "github:${node.owner}/${node.repo}/${node.rev}";
  evaluate = enabled: (import "${pkgs.path}/nixos/lib/eval-config.nix" {
    inherit system;
    specialArgs = { inherit inputs; };
    modules = [
      ../../../config/settings.nix
      ../../../config/system/nixos/opportunistic-unlock.nix
      {
        settings.disk.encryption.opportunisticUnlock.enable = enabled;
        settings.disk.label.root = "testroot";
        system.stateVersion = "24.11";
      }
    ];
  }).config;
  enabled = evaluate true;
  disabled = evaluate false;
in
assert !enabled.services.udisks2.enable;
assert enabled.systemd.services.nixos-opportunistic-unlock.serviceConfig.Type == "exec";
assert enabled.systemd.services.nixos-opportunistic-unlock.serviceConfig.MemorySwapMax == 0;
assert builtins.elem "x-nixos.opportunistic-passphrase" enabled.boot.initrd.luks.devices.testroot.crypttabExtraOpts;
assert !(disabled.systemd.services ? nixos-opportunistic-unlock);
import ../../../config/lib/btrc.nix { inherit pkgs inputs; } {
  name = "opportunistic-unlock-policy-tests";
  src = pkgs.lib.fileset.toSource {
    root = ../../..;
    fileset = pkgs.lib.fileset.unions [
      ./btrc.toml ./Headers.btrc ./PolicyTests.btrc ./MemoryTests.btrc
      ../../../config/system/nixos/opportunistic-unlock
    ];
  };
  programs = {
    policy-tests = "tests/opportunistic-unlock/unit/PolicyTests.btrc";
    header-tests = "tests/opportunistic-unlock/unit/Headers.btrc";
    memory-tests = "tests/opportunistic-unlock/unit/MemoryTests.btrc";
    worker-contract = "config/system/nixos/opportunistic-unlock/Main.btrc";
  };
  nativeBuildInputs = [ pkgs.cryptsetup ];
  buildInputs = [ pkgs.cryptsetup ];
  linkFlags = [ "-lcryptsetup" ];
  postInstall = ''
  ./policy-tests
  ./memory-tests
  # This process must not acquire a managed string/ARC heap or runtime threads.
  # Its raw mmap buffer is never registered, retained, copied or destroyed by BTRC.
  if grep -Eq '__btrc_(string|arc|cleanup|throw)|pthread_create|pthread_mutex' worker-contract.c; then
    echo 'unexpected managed or threaded runtime in the secret-bearing worker' >&2
    exit 1
  fi
  grep -q 'explicit_bzero(secret, secretBytes)' worker-contract.c
  grep -q 'explicit_bzero(secret, passwordLimit)' worker-contract.c
  nm -D worker-contract | grep -q explicit_bzero
  printf fixture-password > passphrase
  for format in luks1 luks2; do
    truncate -s 32M "$format.img"
    cryptsetup luksFormat --batch-mode --type "$format" --pbkdf pbkdf2 --iter-time 1 --key-file passphrase "$format.img"
    ./header-tests "$format.img" 1
  done
  truncate -s 32M argon.img
  cryptsetup luksFormat --batch-mode --type luks2 --pbkdf argon2id --pbkdf-memory 8192 --pbkdf-parallel 1 --pbkdf-force-iterations 4 --key-file passphrase argon.img
  ./header-tests argon.img 1
  truncate -s 32M unsupported.img
  cryptsetup luksFormat --batch-mode --type luks1 --cipher aes-cbc-plain64 --key-size 256 --pbkdf pbkdf2 --iter-time 1 --key-file passphrase unsupported.img
  ./header-tests unsupported.img 0
  truncate -s 32M malformed.img
  printf 'LUKS\272\276\000\002' > malformed.img
  ./header-tests malformed.img 0
  cryptsetup luksErase --batch-mode luks2.img
  ./header-tests luks2.img 0
  '';
}
