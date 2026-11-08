{ flake ? builtins.getFlake "git+file://${toString ../../..}", system ? builtins.currentSystem, renderNode ? "/dev/dri/renderD128", realSuspend ? false, withSunshine ? true }:
(import ../gamescope-native {
  inherit flake system renderNode;
  settings = { mode = "lock"; inherit realSuspend withSunshine; };
  extraModules = [ ({ lib, ... }: {
    name = lib.mkForce (if realSuspend then "sleep-gamescope-native-resume-lock" else "sleep-gamescope-native-lock" + lib.optionalString (!withSunshine) "-without-sunshine");
    node.pkgsReadOnly = false;
    nodes.machine = { config, pkgs, ... }: let
      input = pkgs.runCommandCC "gamescope-lock-input" { } ''
        mkdir -p $out/bin
        $CC -Wall -Wextra -Werror ${../../plymouth-keyboard/input.c} -o $out/bin/test-input
      '';
    in {
      # Keep the baseline's Jovian packages when extending this node's modules.
      nixpkgs.overlays = [ flake.inputs.jovian.overlays.default ];
      nixpkgs.config.allowUnfree = true;
      settings.sleep.lockGamingSession = true;
      users.users.observer.password = lib.mkForce "aB3! z";
      # Test the real user-manager stop/start/condition boundary without requiring
      # pairing credentials or a stream. Pixel/input tests use real Gamescope.
      services.sunshine = { enable = withSunshine; autoStart = false; };
      systemd.user.services = lib.optionalAttrs withSunshine {
        sunshine.serviceConfig = {
          Type = lib.mkForce "simple";
          ExecStart = lib.mkForce [ "" "${pkgs.coreutils}/bin/sleep infinity" ];
        };
        "app-dev.lizardbyte.app.Sunshine".serviceConfig = {
          Type = "simple";
          ExecStart = "${pkgs.coreutils}/bin/sleep infinity";
        };
      };
      boot.kernelModules = [ "uinput" ];
      environment.systemPackages = [ input ];
      services.udev.extraRules = ''
        SUBSYSTEM=="input", KERNEL=="event*", ATTRS{name}=="AT Translated Set 2 keyboard", ENV{PLYMOUTH_IGNORE_KEYBOARD}="1"
        SUBSYSTEM=="input", KERNEL=="event*", ATTRS{name}=="QEMU Virtio Keyboard", ENV{PLYMOUTH_IGNORE_KEYBOARD}="1"
      '';
      systemd.services.test-input = {
        wantedBy = [ "multi-user.target" ];
        serviceConfig.ExecStart = "${input}/bin/test-input touchscreen";
      };
      services.logind.settings.Login.HandlePowerKey = "ignore";
      # Keep production pre/post lock hooks. Only the sleep backend is
      # injectable; the last scenario delegates to real kernel s2idle.
      systemd.services.systemd-suspend.serviceConfig.ExecStart = lib.mkForce [ ""
        "${pkgs.writeShellScript "gamescope-sleep-backend" ''
          if test -e /run/expect-gaming-lock; then
            test "$(cat /sys/class/tty/tty0/active)" = tty63 || exit 90
          fi
          touch /run/sleep-backend-entered
          if test -e /run/real-suspend; then
            exec ${config.systemd.package}/lib/systemd/systemd-sleep suspend
          fi
          while test -e /run/hold-sleep-backend; do sleep .1; done
          if test -e /run/fail-sleep-backend; then exit 1; fi
        ''}"
      ];
    };
  }) ];
})
