{ config, lib, pkgs, inputs, ... }:
let
  inherit (import ./sleep/packages.nix { inherit pkgs inputs; plymouth = config.boot.plymouth.package; }) blank native;
  configuration = pkgs.writeText "sleep-lock.json" (builtins.toJSON {
    user = config.settings.users.admin.username;
    blank = "${blank}/bin/sleep-lock-blank";
    native = "${native}/bin/sleep-lock-native";
    plymouth = "${config.boot.plymouth.package}/bin/plymouth";
    plymouthd = "${config.boot.plymouth.package}/bin/plymouthd";
    systemctl = "${config.systemd.package}/bin/systemctl";
    loginctl = "${config.systemd.package}/bin/loginctl";
    notify = "${config.systemd.package}/bin/systemd-notify";
    setpriv = "${pkgs.util-linux}/bin/setpriv";
    env = "${pkgs.coreutils}/bin/env";
  });
  controllerPackage = (import ../../lib/btrc.nix { inherit pkgs inputs; }) {
    name = "sleep-lock";
    src = lib.fileset.toSource {
      root = ../..;
      fileset = lib.fileset.unions [
        ./sleep/Main.btrc ./sleep/Controller.btrc ./sleep/State.btrc
        ../../lib/BackgroundProcess.btrc
        ../../pkgs/utils/nixosctl/lib/RootSecretFile.btrc
      ];
    };
    programs.sleep-lock = "system/nixos/sleep/Main.btrc";
    wrapperArgs = [ "--add-flags" (toString configuration) ];
  };
  controller = "${controllerPackage}/bin/sleep-lock";
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
