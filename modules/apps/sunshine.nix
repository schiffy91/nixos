{ lib, config, ... }: {
  nixpkgs.overlays = [
    (final: prev: {
      sunshine = final.callPackage ./pkg-overrides/sunshine/package.nix { cudaSupport = true; };
    })
  ];

  # cudaSupport=true bakes /run/opengl-driver/lib into RUNPATH so LD_LIBRARY_PATH
  # isn't strictly required, but keep it set for the upstream-shipped service unit
  # in case the wrapper path drops it.
  systemd.user.services = lib.mkIf config.services.sunshine.enable {
    sunshine.environment.LD_LIBRARY_PATH = "/run/opengl-driver/lib";
    "app-dev.lizardbyte.app.Sunshine".environment.LD_LIBRARY_PATH = "/run/opengl-driver/lib";
  };
}
