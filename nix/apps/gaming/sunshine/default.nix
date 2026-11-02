{ lib, config, ... }:
let
  enabled = config.settings.apps.enable && config.settings.apps.gaming.enable && config.settings.apps.sunshine.enable;
  capture = config.settings.apps.sunshine.capture;
in {
  nixpkgs.overlays = lib.optional (enabled && config.settings.apps.sunshine.cuda.enable)
    (_final: prev: {
      sunshine = (prev.sunshine.override { cudaSupport = true; }).overrideAttrs (old: {
        # cudaSupport wraps $out/bin/sunshine in a shell script that exec's
        # .sunshine-wrapped. KWin's screencast permission gate matches Exec=
        # against /proc/self/exe, which is the inner ELF after exec.
        postFixup = (old.postFixup or "") + ''
          substituteInPlace $out/share/applications/dev.lizardbyte.app.Sunshine.kwin.desktop \
            --replace-fail "Exec=$out/bin/sunshine" "Exec=$out/bin/.sunshine-wrapped"
        '';
      });
    })
  # Backport of KDE MR !8293 (HDR screencast support), still unmerged upstream.
  # Adds 10-bit DRM formats + BT.2020/SMPTE2084 negotiation in the PipeWire
  # screencast stream so Sunshine can capture genuine HDR pixels from the
  # compositor instead of 8-bit-truncated SDR.
  # https://invent.kde.org/plasma/kwin/-/merge_requests/8293
  ++ lib.optional (enabled && capture == "kwin")
    (_final: prev: {
      kdePackages = prev.kdePackages.overrideScope (_kdeFinal: kdePrev: {
        kwin = kdePrev.kwin.overrideAttrs (old: {
          patches = (old.patches or []) ++ [ ./hdr-screencast.patch ];
        });
      });
    });

  services.sunshine = lib.mkIf enabled {
    enable = true;
    openFirewall = false;
    autoStart = true;
    capSysAdmin = capture == "kms";  # gamescope has no screencast portal; KMS capture reads the scanout directly
    settings.capture = capture;
  };
  settings.networking.ports = lib.mkIf enabled {
    tcp = [ 47984 47989 47990 48010 ];
    udp = (lib.range 47998 48000) ++ (lib.range 8000 8010);
  };

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
