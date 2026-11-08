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
  controller = pkgs.writeScript "sleep-lock" ("#!${pkgs.python3}/bin/python3\n" +
    builtins.replaceStrings
      [ "@blank@" "@native@" "@plymouth@" "@plymouthd@" "@systemctl@" "@loginctl@" "@user@" ]
      [ "${blank}/bin/sleep-lock-blank" "${native}/bin/sleep-lock-native" "${config.boot.plymouth.package}/bin/plymouth"
        "${config.boot.plymouth.package}/bin/plymouthd" "${config.systemd.package}/bin/systemctl"
        "${config.systemd.package}/bin/loginctl" config.settings.users.admin.username ]
      (builtins.readFile ./sleep/lock.py));
  sleepServices = [ "systemd-suspend" "systemd-hibernate" "systemd-hybrid-sleep" "systemd-suspend-then-hibernate" ];
in lib.mkIf config.settings.sleep.lockGamingSession {
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
  # Normal service starts/restarts cannot reopen a stream while Gaming Mode is
  # locked. The root controller stops existing instances before acknowledging
  # the sleep request and only restores instances which were running before it.
  systemd.user.services = lib.optionalAttrs config.services.sunshine.enable
    (lib.genAttrs [ "sunshine" "app-dev.lizardbyte.app.Sunshine" ] (_: {
      unitConfig.ConditionPathExists = "!/run/nixos-gaming-locked";
      serviceConfig.KillMode = "control-group";
    }));
  systemd.services = lib.genAttrs sleepServices (_: {
    serviceConfig = {
      ExecStartPre = lib.mkBefore [ "${controller} prepare %n" ];
      ExecStopPost = lib.mkAfter [ "${controller} resume %n" ];
    };
  }) // {
    nixos-sleep-lock = {
      description = "Account password lock for sleeping Gaming Mode sessions";
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
