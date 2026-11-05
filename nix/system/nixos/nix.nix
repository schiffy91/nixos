{ config, ... }:
{
  nix.settings = {
    extra-substituters = config.settings.nix.substituters;
    extra-trusted-public-keys = config.settings.nix.trustedPublicKeys;
    connect-timeout = 5;  # a publisher that is off must not stall a build
    fallback = true;      # and must not fail one either
  };
}
