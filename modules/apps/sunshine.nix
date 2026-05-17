{ lib, config, ... }: {
  nixpkgs.overlays = [
    (final: prev: {
      sunshine = final.callPackage ./pkg-overrides/sunshine/package.nix { cudaSupport = true; };
    })
  ];

  # KWin only exposes enabled outputs via screencast, so the streaming display must
  # be live before Sunshine's encoder probe runs at service start.
  systemd.user.services = lib.mkIf config.services.sunshine.enable {
    sunshine = {
      environment.LD_LIBRARY_PATH = "/run/opengl-driver/lib";
      serviceConfig.ExecStartPre = "/run/current-system/sw/bin/sunshine-display-enable";
    };
    "app-dev.lizardbyte.app.Sunshine" = {
      environment.LD_LIBRARY_PATH = "/run/opengl-driver/lib";
      serviceConfig.ExecStartPre = "/run/current-system/sw/bin/sunshine-display-enable";
    };
  };
}
