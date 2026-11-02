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
    });
  # PARKED on 2026-11-02: ./hdr-screencast.patch is our backport of KDE
  # MR !8293 (10-bit DRM formats plus BT.2020/SMPTE2084 negotiation in the
  # PipeWire screencast stream, so Sunshine captures real HDR pixels instead of
  # 8-bit-truncated SDR). It applied to Plasma 6.7.5; Plasma 6.8.2 refactored
  # the screencast plugin (RenderDevice/EglContext, setNextPresentationTimestamp,
  # no ScreenCastSource::clock) and seven hunks across four files now reject.
  # MR !8293 is still unmerged and untouched since 2026-09-04, so its current
  # diff does not apply to 6.8.2 either, and 6.8.2 has no HDR screencast support
  # of its own. Re-enable this overlay once the patch is ported and HDR capture
  # is verified against a real Sunshine session; until then KWin capture works
  # in SDR. https://invent.kde.org/plasma/kwin/-/merge_requests/8293

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
