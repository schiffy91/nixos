{ config, lib, pkgs, ... }:
{
  config = lib.mkIf config.services.buffyboard.enable {
    # Buffyboard writes console keystrokes through uinput. Minimal recovery
    # images have no compositor or controller service to load it incidentally.
    boot.kernelModules = [ "uinput" ];
    services.udev.extraRules = ''
      SUBSYSTEM=="graphics", KERNEL=="fb0", TAG+="systemd"
    '';
    services.buffyboard.settings.theme.default = "breezy-dark";
    # Getty/login can reset the console after Buffyboard's first draw. The
    # login shell is the point at which its VT is open and ready for input.
    environment.loginShellInit = lib.mkAfter ''
      case "$(${pkgs.coreutils}/bin/tty 2>/dev/null)" in
        /dev/tty[0-9]*)
          ${pkgs.systemd}/bin/systemctl start buffyboard.service
          ${pkgs.systemd}/bin/systemctl kill --signal=SIGUSR1 buffyboard.service
          ;;
      esac
    '';
    systemd.services.buffyboard = {
      requires = [ "dev-fb0.device" ];
      wants = [ "systemd-modules-load.service" ];
      after = [ "dev-fb0.device" "systemd-modules-load.service" ];
      unitConfig.StartLimitIntervalSec = 0;
      serviceConfig.RestartSec = 1;
    };
  };
}
