{ config, pkgs, lib, ... }:
let
  primary = lib.findFirst (o: o.primary) null config.settings.desktop.outputs;
  primaryScale = if primary == null then 1.0 else primary.scaleFactor;
in {
  # Backport of KDE MR !8293 (HDR screencast support) to KWin 6.6.5.
  # Adds 10-bit DRM formats + BT.2020/SMPTE2084 negotiation in the PipeWire
  # screencast stream so Sunshine can capture genuine HDR pixels from the
  # compositor instead of 8-bit-truncated SDR.
  nixpkgs.overlays = [
    (final: prev: {
      kdePackages = prev.kdePackages.overrideScope (kdeFinal: kdePrev: {
        kwin = kdePrev.kwin.overrideAttrs (old: {
          patches = (old.patches or []) ++ [ ./pkg-overrides/kwin/hdr-screencast.patch ];
        });
      });
    })
  ];

  hardware.graphics.enable = true;
  programs.dconf.enable = true;
  services = {
    xserver.dpi = builtins.floor (96 * primaryScale);
    displayManager = {
      enable = true;
      sddm = {
        enable = true;
        wayland.enable = true;
        theme = "breeze";  # Plasma 6 Breeze SDDM theme; autofocuses password on single-user systems
      };
      defaultSession = "plasma";
    };
    desktopManager.plasma6 = {
      enable = true;
      enableQt5Integration = false;
    };
    accounts-daemon.enable = true;
  };
  environment = {
    systemPackages = (with pkgs; [
      wl-clipboard
      config.settings.desktop.cursor.package
      config.settings.desktop.cursor.defaultPackage
    ]) ++ (with pkgs.kdePackages; [
      plasma-thunderbolt
      kaccounts-providers
      kaccounts-integration
      kio-gdrive
    ]);
    sessionVariables = {
      XCURSOR_THEME = config.settings.desktop.cursor.theme;
      NIXOS_OZONE_WL = "1";
    };
    plasma6.excludePackages = with pkgs.kdePackages; [
      kate
      khelpcenter
      elisa
      okular
      print-manager
    ];
  };
  security.pam.services.sddm.enableKwallet = config.settings.user.admin.autoUnlockWallet.enabled;
  security.pam.services.passwd.enableKwallet = config.settings.user.admin.autoUnlockWallet.enabled;  # pam_kwallet hooks chauthtok → wallet re-encrypts on passwd
}
