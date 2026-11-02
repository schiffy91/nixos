{ config, inputs, lib, pkgs, ... }:
let
  # The overlay condition must not read pkgs: pkgs is built from the overlays.
  declared = config.settings.apps.enable && config.settings.apps.gaming.enable && config.settings.apps.simracing.enable;
  enabled = declared && pkgs.stdenv.hostPlatform.isx86_64;
in {
  imports = [ inputs.logitech-trueforce.nixosModules.default ];

  # nixpkgs now signs every kernel module with kmodigest (kernel makeFlags set
  # `cmd_sign`) and the kmod aggregator cats each external module's sibling
  # `<mod>.ko.cer` into modules.cer. The logitech-trueforce flake installs its
  # module with a plain `cp` and overrides makeFlags, so `cmd_sign` never
  # reaches kbuild, no certificate is written, and the aggregator dies with
  # `cat: extra/hid-logitech-dd.ko.cer: No such file or directory`. The
  # certificate is a per-module digest, so it cannot be supplied separately.
  #
  # Tolerate a missing certificate instead. This host checks module signatures
  # but does not require them: CONFIG_MODULE_SIG_FORCE is unset and lockdown is
  # not in `lsm=`, so modules.cer only seeds the trusted-key list and the module
  # loads exactly as it did before nixpkgs added signing. Drop this overlay once
  # the driver's flake passes the kernel's `cmd_sign` through to kbuild.
  config.nixpkgs.overlays = lib.optional declared (_final: prev: {
    aggregateModules = args: (prev.aggregateModules args).overrideAttrs (old: {
      postBuild = builtins.replaceStrings
        [ "| xargs -d'\\n' cat >> modules.cer" ]
        [ "| xargs -d'\\n' -r sh -c 'for cert; do test -e \"$cert\" && cat -- \"$cert\"; done' sh >> modules.cer" ]
        old.postBuild;
    });
  });

  config.hardware.logitech-trueforce.enable = lib.mkIf enabled true;
}
