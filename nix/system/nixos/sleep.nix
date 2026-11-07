{ config, lib, pkgs, ... }:
let
  blank = pkgs.runCommandCC "sleep-lock-blank" {
    nativeBuildInputs = [ pkgs.pkg-config ];
    buildInputs = [ pkgs.libdrm ];
  } ''
    mkdir -p $out/bin
    $CC -std=c11 -O2 -Wall -Wextra -Werror ${./sleep/blank.c} \
      $(pkg-config --cflags --libs libdrm) -o $out/bin/sleep-lock-blank
  '';
  native = pkgs.runCommandCC "sleep-lock-native" { buildInputs = [ pkgs.pam ]; } ''
    mkdir -p $out/bin
    $CC -std=c11 -O2 -Wall -Wextra -Werror \
      -DPLYMOUTH='"${config.boot.plymouth.package}/bin/plymouth"' \
      ${./sleep/native.c} -lpam -o $out/bin/sleep-lock-native
  '';
  plasma = pkgs.writeText "sleep-lock-plasma.py" (builtins.replaceStrings
    [ "@loginctl@" "@busctl@" ]
    [ "${config.systemd.package}/bin/loginctl" "${config.systemd.package}/bin/busctl" ]
    (builtins.readFile ./sleep/plasma.py));
  controller = pkgs.writeScript "sleep-lock" ("#!${pkgs.python3}/bin/python3\n" +
    builtins.replaceStrings
      [ "@plasma@" "@blank@" "@native@" "@plymouth@" "@plymouthd@" "@systemctl@" "@user@" ]
      [ "${plasma}" "${blank}/bin/sleep-lock-blank" "${native}/bin/sleep-lock-native" "${config.boot.plymouth.package}/bin/plymouth"
        "${config.boot.plymouth.package}/bin/plymouthd" "${config.systemd.package}/bin/systemctl"
        config.settings.users.admin.username ]
      (builtins.readFile ./sleep/lock.py));
  sleepServices = [ "systemd-suspend" "systemd-hibernate" "systemd-hybrid-sleep" "systemd-suspend-then-hibernate" ];
in lib.mkIf config.settings.sleep.lockSession {
  # Session brokers admit their owner by default. The native lock API checks
  # the real sender UID, so explicitly admit root without impersonating the
  # desktop account or opening the bus to any other user.
  services.dbus.packages = [ (pkgs.writeTextDir "share/dbus-1/session.d/nixos-sleep-lock.conf" ''
    <busconfig>
      <policy context="mandatory">
        <allow user="root"/>
      </policy>
    </busconfig>
  '') ];
  nixpkgs.overlays = [ (_: previous: {
    kdePackages = previous.kdePackages.overrideScope (_: kdePrevious: {
      kscreenlocker = kdePrevious.kscreenlocker.overrideAttrs (old: {
        patches = (old.patches or [ ]) ++ [ ./sleep/kscreenlocker-fail-closed.patch ];
      });
      kwin = kdePrevious.kwin.overrideAttrs (old: {
        patches = (old.patches or [ ]) ++ [
          ./sleep/kwin-lock-after-crash.patch
          ./sleep/kwin-resume-paused-device.patch
          ./sleep/kwin-suspend-inactive-input.patch
        ];
      });
    });
  }) ];
  assertions = [
    { assertion = config.boot.plymouth.enable; message = "Session locking requires Plymouth."; }
    { assertion = !config.services.physlock.enable; message = "Use one owner for the session console lock."; }
    { assertion = builtins.match "[a-zA-Z_][a-zA-Z0-9_-]*" config.settings.users.admin.username != null;
      message = "Session locking requires a local administrator account name."; }
  ];
  security.pam.services.nixos-sleep-lock.text = ''
    auth required ${config.security.pam.pam_unixModulePath}
    account required ${config.security.pam.pam_unixModulePath}
  '';
  # Native Plasma locking also conceals software/virtual outputs. A no-password
  # screensaver or grace-time dismissal cannot serve as this boundary. These
  # keys do not change the separately configured automatic idle-lock policy.
  environment.etc."xdg/kscreenlockerrc".text = ''
    [Daemon]
    Lock[$i]=true
    LockGrace[$i]=0
    RequirePassword[$i]=true
  '';
  systemd.services = lib.genAttrs sleepServices (_: {
    serviceConfig = {
      ExecStartPre = lib.mkBefore [ "${controller} prepare %n" ];
      ExecStopPost = lib.mkAfter [ "${controller} resume %n" ];
    };
  }) // {
    nixos-sleep-lock = {
      description = "Account password lock for sleeping desktop and gaming sessions";
      after = [ "plymouth-quit.service" ];
      unitConfig.StartLimitIntervalSec = 0;
      environment = {
        LANG = "C.UTF-8";
        XKB_CONFIG_ROOT = "${pkgs.xkeyboard-config}/share/X11/xkb";
        XLOCALEDIR = "${pkgs.libx11}/share/X11/locale";
      };
      serviceConfig = {
        Type = "notify";
        ExecStart = "${controller} run";
        Restart = "on-failure";
        RestartForceExitStatus = [ "SIGTERM" "SIGINT" ];
        RestartSec = 1;
        TimeoutStartSec = 20;
        TimeoutStopSec = 5;
        UMask = "0077";
        LimitCORE = 0;
        LimitMEMLOCK = "infinity";
      };
    };
  };
}
