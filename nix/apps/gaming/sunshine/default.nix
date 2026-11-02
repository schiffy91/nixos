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
  # Backport of KDE MR !8293, rebased onto Plasma 6.8.2 and fixed up, so the
  # stream can negotiate BT.2020 + SMPTE2084 and Sunshine captures real HDR.
  # Without it KWin lets a client negotiate PQ and then hands it SDR pixels.
  # Upstream is unmerged: https://invent.kde.org/plasma/kwin/-/merge_requests/8293
  # Deviations from the MR, each verified by ./hdr-screencast-test:
  #   - apply the negotiated colour description on the memfd path too, not only
  #     on dmabuf, otherwise a PQ memfd stream carries unconverted sRGB pixels
  #   - announce every allocatable dmabuf format rather than only the preferred
  #     one, so 8-bit dmabuf clients keep zero-copy and HDR clients can pick
  #   - adopt whichever offered format the client chose
  #   - announce the alpha-less 10-bit twin and allocate the alpha-carrying one;
  #     drivers refuse to render into XBGR2101010 but accept ABGR2101010, and
  #     Sunshine only requests BT.2020 + PQ for xBGR_210LE
  #   - fall back to memfd when no dmabuf format can be allocated, instead of
  #     failing the stream outright
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
