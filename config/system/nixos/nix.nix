{ config, lib, ... }:
let
  store = config.settings.nix;
  gib = 1024 * 1024 * 1024;
in {
  assertions = [ {
    assertion = store.minFreeGiB == 0 || store.maxFreeGiB > store.minFreeGiB;
    message = "settings.nix.maxFreeGiB must be above settings.nix.minFreeGiB, or a build collects garbage and frees nothing";
  } ];
  settings.disk.immutability.persist.paths = [ "/root/.cache/nix/" ];
  nix.settings = {
    extra-substituters = config.settings.nix.substituters;
    extra-trusted-public-keys = config.settings.nix.trustedPublicKeys;
    connect-timeout = 5;  # a publisher that is off must not stall a build
    fallback = true;      # and must not fail one either
    warn-dirty = false;   # upgrades rewrite the locks before building
  } // lib.optionalAttrs (store.minFreeGiB > 0) {
    min-free = store.minFreeGiB * gib;  # dead paths once filled a disk
    max-free = store.maxFreeGiB * gib;
  };
}
