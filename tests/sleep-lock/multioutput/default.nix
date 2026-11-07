{ system ? builtins.currentSystem }:
(import ../authentication { inherit system; }).extend {
  modules = [ ({ lib, ... }: {
    name = lib.mkForce "sleep-lock-multioutput";
    nodes.machine = { config, pkgs, ... }: let
      faults = pkgs.runCommandCC "sleep-renderer-faults" {
        nativeBuildInputs = [ pkgs.pkg-config ];
        buildInputs = [ pkgs.libdrm ];
      } ''
        mkdir -p $out/lib $out/bin
        $CC -Wall -Wextra -Werror -shared -fPIC ${./fault.c} \
          $(pkg-config --cflags --libs libdrm) -ldl -o $out/lib/fault.so
        $CC -Wall -Wextra -Werror ${./blank-check.c} \
          $(pkg-config --cflags --libs libdrm) -o $out/bin/drm-blank-check
      '';
      plymouth = pkgs.callPackage ../../../nix/system/nixos/boot/plymouth/package.nix {
        systemd = config.boot.initrd.systemd.package;
      };
      faultPlymouth = pkgs.symlinkJoin {
        name = "plymouth-test-faults";
        paths = [ plymouth ];
        postBuild = ''
          rm $out/bin/plymouthd
          cat > $out/bin/plymouthd <<'EOF'
          #!${pkgs.runtimeShell}
          export LD_PRELOAD=${faults}/lib/fault.so
          exec ${plymouth}/bin/plymouthd "$@"
          EOF
          chmod +x $out/bin/plymouthd
        '';
      };
      client = pkgs.stdenv.mkDerivation {
        name = "sleep-multioutput-client";
        dontUnpack = true;
        nativeBuildInputs = [ pkgs.pkg-config pkgs.qt6.wrapQtAppsHook ];
        buildInputs = [ pkgs.qt6.qtbase pkgs.qt6.qtwayland ];
        buildPhase = ''
          $CXX -Wall -Wextra -Werror -fPIC ${./client.cpp} -o client $(pkg-config --cflags --libs Qt6Widgets)
        '';
        installPhase = ''mkdir -p $out/bin; cp client $out/bin/sleep-multioutput-client'';
      };
    in {
      boot.plymouth.package = lib.mkForce faultPlymouth;
      # Keep the boot-time hint used by Fractal-North: sleeping sessions must
      # still cover both cards. The pinned Plymouth does not consume this hint.
      boot.kernelParams = [ "plymouth.drm-device=/dev/dri/card0" ];
      virtualisation.qemu.options = lib.mkAfter [
        "-vga none"
        "-device virtio-vga,id=graphics0,x-pcie-pm-no-soft-reset=on"
        "-device virtio-gpu-pci,id=graphics1,x-pcie-pm-no-soft-reset=on"
      ];
      environment.systemPackages = [ client faults pkgs.kdePackages.kscreen ];
      # Distinguish identical Virtual-1 connector names on separate DRM cards.
      services.udev.extraRules = ''
        SUBSYSTEM=="input", KERNEL=="event*", ATTRS{name}=="Plymouth test touchscreen", ENV{WL_OUTPUT}="card0-Virtual-1"
      '';
    };
    testScript = lib.mkForce (builtins.readFile ./test.py);
  }) ];
}
