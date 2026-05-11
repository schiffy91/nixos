{ lib, config, ... }: {
  nixpkgs.overlays = [
    (final: prev: {
      sunshine = final.callPackage ../../pkgs/sunshine/package.nix { };
    })
  ];

  # Without LD_LIBRARY_PATH=/run/opengl-driver/lib sunshine can't dlopen
  # libcuda.so.1 → NVENC init fails → silent fallback to libx264 software
  # encoding → no usable Moonlight stream at 6K. autoAddDriverRunpath misses
  # this because the dependency is dynamic (dlopen, not NEEDED).
  # NOTE: capSysAdmin must be off (FRACTAL-NORTH) otherwise the security
  # wrapper enables AT_SECURE and the dynamic linker strips LD_LIBRARY_PATH.
  systemd.user.services.sunshine.environment = lib.mkIf config.services.sunshine.enable {
    LD_LIBRARY_PATH = "/run/opengl-driver/lib";
  };
}
