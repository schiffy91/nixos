{ system ? builtins.currentSystem, lockSession ? true, softwareRenderAccess ? true }:
(import ../authentication { inherit system lockSession; }).extend {
  modules = [ ({ lib, ... }: {
    name = lib.mkForce "sleep-lock-capture";
    nodes.machine = { pkgs, ... }: let
      probe = pkgs.stdenv.mkDerivation {
        name = "sleep-capture-probe";
        dontUnpack = true;
        nativeBuildInputs = [ pkgs.pkg-config pkgs.wayland-scanner ];
        buildInputs = [ pkgs.wayland pkgs.pipewire ];
        buildPhase = ''
          protocol=${pkgs.kdePackages.plasma-wayland-protocols}/share/plasma-wayland-protocols/zkde-screencast-unstable-v1.xml
          wayland-scanner client-header "$protocol" zkde-screencast-unstable-v1-client-protocol.h
          wayland-scanner private-code "$protocol" protocol.c
          $CC -Wall -Wextra -Werror -I. ${./probe.c} protocol.c -o sleep-capture-probe $(pkg-config --cflags --libs wayland-client libpipewire-0.3)
        '';
        installPhase = ''
          mkdir -p $out/bin $out/share/applications
          cp sleep-capture-probe $out/bin/
          cat > $out/share/applications/sleep-capture-probe.desktop <<DESKTOP
          [Desktop Entry]
          Name=Sleep capture probe
          Type=Application
          Exec=$out/bin/sleep-capture-probe
          NoDisplay=true
          X-KDE-Wayland-Interfaces=zkde_screencast_unstable_v1
          DESKTOP
        '';
      };
      client = pkgs.stdenv.mkDerivation {
        name = "sleep-capture-client";
        dontUnpack = true;
        nativeBuildInputs = [ pkgs.pkg-config pkgs.qt6.wrapQtAppsHook ];
        buildInputs = [ pkgs.qt6.qtbase pkgs.qt6.qtwayland ];
        buildPhase = ''$CXX -Wall -Wextra -Werror -fPIC ${../multioutput/client.cpp} -o client $(pkg-config --cflags --libs Qt6Widgets)'';
        installPhase = ''mkdir -p $out/bin; cp client $out/bin/sleep-capture-client'';
      };
      place = pkgs.writeShellApplication {
        name = "sleep-capture-place";
        runtimeInputs = [ pkgs.systemd pkgs.jq ];
        text = ''
          script_id="$(busctl --user --json=short call org.kde.KWin /Scripting org.kde.kwin.Scripting loadScript ss ${./place.js} sleep-capture-placement | jq -er '.data[0] | select(. >= 0)')"
          trap 'busctl --user call org.kde.KWin /Scripting org.kde.kwin.Scripting unloadScript s sleep-capture-placement >/dev/null' EXIT
          busctl --user call org.kde.KWin "/Scripting/Script$script_id" org.kde.kwin.Script run
        '';
      };
    in {
      # An inactive compositor restart loses uaccess ACLs. Keep the VM's
      # supported software allocator available while another VT is active;
      # kms_swrast on virtio's render node cannot allocate dumb buffers.
      users.users.observer.extraGroups = lib.optional softwareRenderAccess "render";
      services.udev.extraRules = lib.optionalString softwareRenderAccess ''
        KERNEL=="udmabuf", GROUP="render", MODE="0660"
      '';
      services.pipewire.enable = true;
      # Hold a real successful native PAM transaction until after root prepare.
      security.pam.services.kde.text = lib.mkForce ''
        auth requisite ${pkgs.pam}/lib/security/pam_unix.so
        auth required ${pkgs.pam}/lib/security/pam_exec.so quiet ${pkgs.writeShellScript "native-pam-gate" ''
          if test -e /tmp/native-pam-hold; then
            ${pkgs.coreutils}/bin/touch /tmp/native-pam-entered
            while test -e /tmp/native-pam-hold; do ${pkgs.coreutils}/bin/sleep .05; done
            ${pkgs.coreutils}/bin/touch /tmp/native-pam-finished
          fi
        ''}
        account required ${pkgs.pam}/lib/security/pam_unix.so
      '';
      environment.systemPackages = [ probe client place pkgs.kdePackages.kscreen (pkgs.python3.withPackages (p: [ p.pillow ])) ];
      environment.etc."capture-frame.py".source = ./frame.py;
      environment.etc."capture-greeter".text = "${pkgs.kdePackages.kscreenlocker}/libexec/kscreenlocker_greet";
      systemd.services.systemd-suspend.serviceConfig.ExecStartPre = lib.mkAfter [
        "${pkgs.writeShellScript "capture-at-sleep-ack" ''
          ${pkgs.python3.withPackages (p: [ p.pillow ])}/bin/python ${./frame.py} > /run/capture-at-backend.json
        ''}"
      ];
    };
    testScript = lib.mkForce (builtins.readFile ./test.py);
  }) ];
}
