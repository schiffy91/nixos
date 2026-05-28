{ lib, config, ... }:
let
  enabled = config.settings.apps.enable && config.settings.apps.sunshine.enable;
in {
  nixpkgs.overlays = lib.optional enabled
    (final: prev: {
      sunshine = if final.stdenv.hostPlatform.isx86_64
        then final.callPackage ./pkg-overrides/sunshine/package.nix { cudaSupport = true; }
        else prev.sunshine;
    });

  # cudaSupport=true bakes /run/opengl-driver/lib into RUNPATH so LD_LIBRARY_PATH
  # isn't strictly required, but keep it set for the upstream-shipped service unit
  # in case the wrapper path drops it.
  systemd.user.services = lib.mkIf (enabled && config.services.sunshine.enable) {
    sunshine.environment.LD_LIBRARY_PATH = "/run/opengl-driver/lib";
    "app-dev.lizardbyte.app.Sunshine".environment.LD_LIBRARY_PATH = "/run/opengl-driver/lib";
  };
}
