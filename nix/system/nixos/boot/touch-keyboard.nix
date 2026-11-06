{ config, lib, ... }:
lib.mkIf config.settings.boot.touchKeyboard {
  boot.initrd.unl0kr = {
    enable = true;
    settings = {
      general.animations = true;
      general.backend = "drm";
      keyboard.autohide = false;
      keyboard.layout = "us";
      keyboard.popovers = true;
      textarea.obscured = true;
      theme.default = "breezy-dark";
    };
  };

  boot.initrd.systemd.services.unl0kr-agent = {
    # The password request can precede amdgpu's takeover of the firmware
    # framebuffer. unl0kr chooses a DRM card once, when it starts.
    wants = [ "systemd-modules-load.service" "systemd-udev-trigger.service" ];
    after = [ "systemd-modules-load.service" "systemd-udev-trigger.service" ];
    serviceConfig = {
      ExecStartPre = "${config.boot.initrd.systemd.package}/bin/udevadm settle --timeout=30";
      Restart = "on-failure";
      RestartSec = 1;
    };
    # A transient graphics failure must not exhaust the password agent's
    # start limit and leave a handheld permanently stuck at a text prompt.
    unitConfig.StartLimitIntervalSec = 0;
  };
}
