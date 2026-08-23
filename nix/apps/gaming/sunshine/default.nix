{ lib, config, ... }:
let
  enabled = config.settings.apps.enable && config.settings.apps.gaming.enable && config.settings.apps.sunshine.enable;
in {
  nixpkgs.overlays = lib.optional enabled
    (final: prev: {
      sunshine = if final.stdenv.hostPlatform.isx86_64
        then (prev.sunshine.override { cudaSupport = true; }).overrideAttrs (old: {
          # cudaSupport wraps $out/bin/sunshine in a shell script that exec's
          # .sunshine-wrapped. KWin's screencast permission gate matches Exec=
          # against /proc/self/exe, which is the inner ELF after exec.
          postFixup = (old.postFixup or "") + ''
            substituteInPlace $out/share/applications/dev.lizardbyte.app.Sunshine.kwin.desktop \
              --replace-fail "Exec=$out/bin/sunshine" "Exec=$out/bin/.sunshine-wrapped"
          '';
        })
        else prev.sunshine;
    });

  # cudaSupport=true bakes /run/opengl-driver/lib into RUNPATH so LD_LIBRARY_PATH
  # isn't strictly required, but keep it set for the upstream-shipped service unit
  # in case the wrapper path drops it.
  systemd.user.services = lib.mkIf (enabled && config.services.sunshine.enable) {
    sunshine = {
      unitConfig.ConditionUser = config.settings.users.admin.username;
      environment.LD_LIBRARY_PATH = "/run/opengl-driver/lib";
    };
    "app-dev.lizardbyte.app.Sunshine" = {
      unitConfig.ConditionUser = config.settings.users.admin.username;
      environment.LD_LIBRARY_PATH = "/run/opengl-driver/lib";
    };
  };
}
