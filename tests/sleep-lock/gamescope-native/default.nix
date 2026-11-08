{ flake ? builtins.getFlake "git+file://${toString ../../..}", system ? builtins.currentSystem, renderNode ? "/dev/dri/renderD128" }:
let
  inherit (flake) inputs;
  pkgs = import inputs.nixpkgs { inherit system; config.allowUnfree = true; overlays = [ inputs.jovian.overlays.default ]; };
in pkgs.testers.runNixOSTest {
  name = "sleep-gamescope-native-control";
  enableOCR = true;
  extraPythonPackages = p: [ p.vncdotool ];
  nodes.machine = { config, pkgs, lib, ... }: {
    imports = [ inputs.jovian.nixosModules.default ../../../nix/settings.nix
      ../../../nix/system/nixos/boot/touch-keyboard.nix ../../../nix/system/nixos/sleep.nix ];
    disabledModules = [ "${inputs.jovian}/modules/jovian/overlay.nix" ];
    # First prove that the unmodified direct-DRM Gaming Mode can render/input.
    settings.users.admin.username = "observer";
    settings.boot.touchKeyboard = true;
    boot.plymouth.enable = true;
    boot.plymouth.theme = "bgrt";
    boot.kernelModules = [ "virtio_gpu" ];
    boot.kernelPackages = pkgs.linuxPackages_latest;
    boot.kernelParams = [ "console=tty0" "plymouth.ignore-serial-consoles" "no_console_suspend" ];
    virtualisation.qemu.package = lib.mkForce pkgs.qemu;
    virtualisation.qemu.options = [
      "-nodefaults"
      "-vga none"
      "-device virtio-vga-gl,id=graphics,hostmem=1G,blob=on,drm_native_context=on,x-pcie-pm-no-soft-reset=on"
      "-display egl-headless,rendernode=${renderNode}"
      # QEMU's working directory is private test scratch. This exposes the
      # real guest console through EGL readback, without any network listener.
      "-vnc unix:gamescope.vnc"
    ];
    virtualisation.memorySize = 4096;
    virtualisation.cores = 2;
    virtualisation.useNixStoreImage = true;
    virtualisation.writableStore = true;
    virtualisation.sharedDirectories = lib.mkForce { };
    users.users.observer = { isNormalUser = true; uid = 1000; password = "a"; };
    jovian.steam = {
      enable = true;
      autoStart = true;
      user = "observer";
      desktopSession = "gamescope-wayland";
    };
    hardware.graphics.enable = true;
    networking.networkmanager.enable = true;
    systemd.services.scx.enable = lib.mkForce false;
    # Native context is served by RADV in the guest; Venus remains disabled.
    systemd.user.services.gamescope-session.environment.VK_DRIVER_FILES =
      "/run/opengl-driver/share/vulkan/icd.d/radeon_icd.x86_64.json";
    # Keep pinned SDDM/PAM/logind/Gamescope. Replace only the Steam account
    # application with a real interactive Xwayland client.
    systemd.user.services.steam-launcher = {
      overrideStrategy = "asDropin";
      serviceConfig = {
        ExecStartPre = lib.mkForce [ "" ];
        ExecStopPost = lib.mkForce [ "" ];
        ExecStop = lib.mkForce [ "" ];
        ExecStart = lib.mkForce [ "" "${pkgs.xterm}/bin/xterm -fa monospace -fs 18 -e ${pkgs.writeShellScript "gamescope-control-client" ''
          # Gamescope's real Steam mode ignores untagged X11 clients. Give
          # this Steam replacement the UI app ID before checking scanout.
          ${pkgs.xprop}/bin/xprop -id "$WINDOWID" -f STEAM_GAME 32c -set STEAM_GAME 769
          # Steam also explicitly selects its global base layer; eligibility
          # alone does not select a window in SteamControlled focus mode.
          ${pkgs.xprop}/bin/xprop -root -f GAMESCOPECTRL_BASELAYER_APPID 32c -set GAMESCOPECTRL_BASELAYER_APPID 769
          printf '%s\n' "$WINDOWID" > /tmp/gamescope-control-window
          printf 'GAMESCOPE_CONTROL_READY\n'
          exec ${pkgs.bashInteractive}/bin/bash --noprofile --norc
        ''}" ];
      };
    };
    environment.systemPackages = [ pkgs.vulkan-tools pkgs.xterm pkgs.xprop ];
    systemd.sleep.settings.Sleep.MemorySleepMode = "s2idle";
  };
  testScript = builtins.readFile ./control.py;
}
