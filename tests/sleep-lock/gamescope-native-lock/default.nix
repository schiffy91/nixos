{ flake ? builtins.getFlake "git+file://${toString ../../..}", system ? builtins.currentSystem, renderNode ? "/dev/dri/renderD128", realSuspend ? false }:
(import ../gamescope-native { inherit flake system renderNode; }).extend {
  modules = [ ({ lib, ... }: {
    name = lib.mkForce (if realSuspend then "sleep-gamescope-native-resume-lock" else "sleep-gamescope-native-lock");
    node.pkgsReadOnly = false;
    extraPythonPackages = p: [ p.pillow ];
    nodes.machine = { config, pkgs, ... }: let
      input = pkgs.runCommandCC "gamescope-lock-input" { } ''
        mkdir -p $out/bin
        $CC -Wall -Wextra -Werror ${../../plymouth-keyboard/input.c} -o $out/bin/test-input
      '';
    in {
      # Mutable node packages admit the production lock overlay, so retain
      # the same Jovian package overlay the baseline's read-only pkgs had.
      nixpkgs.overlays = [ flake.inputs.jovian.overlays.default ];
      nixpkgs.config.allowUnfree = true;
      settings.sleep.lockSession = lib.mkForce true;
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
          test "$(cat /sys/class/tty/tty0/active)" = tty63 || exit 90
          touch /run/sleep-backend-entered
          if test -e /run/real-suspend; then
            exec ${config.systemd.package}/lib/systemd/systemd-sleep suspend
          fi
          if test -e /run/fail-sleep-backend; then exit 1; fi
        ''}"
      ];
    };
    testScript = lib.mkForce ("real_suspend = ${if realSuspend then "True" else "False"}\n" + builtins.readFile ./test.py);
  }) ];
}
