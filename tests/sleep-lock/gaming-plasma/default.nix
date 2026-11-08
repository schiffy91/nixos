{ system ? builtins.currentSystem }:
let
  locked = (builtins.fromJSON (builtins.readFile ../../../flake.lock)).nodes.nixpkgs.locked;
  pkgs = import (builtins.fetchTree {
    inherit (locked) type owner repo rev narHash;
  }) { inherit system; };
in pkgs.testers.runNixOSTest {
  node.pkgsReadOnly = false;
  name = "sleep-gaming-plasma";
  enableOCR = true;
  extraPythonPackages = packages: [ packages.pillow ];
  nodes.machine = { pkgs, lib, ... }: {
    imports = [ ../../../nix/settings.nix ../../../nix/system/nixos/boot/touch-keyboard.nix ../../../nix/system/nixos/sleep.nix ];
    settings.boot.touchKeyboard = true;
    settings.sleep.lockGamingSession = true;
    settings.users.admin.username = "observer";
    boot.plymouth.enable = true;
    boot.plymouth.theme = "bgrt";
    boot.kernelPackages = pkgs.linuxPackages_latest;
    boot.kernelModules = [ "virtio_gpu" "uinput" ];
    boot.kernelParams = [ "console=tty0" "plymouth.ignore-serial-consoles" "no_console_suspend" ];
    # Linux 7.3 provides virtio-gpu freeze/restore callbacks; 6.18 loses
    # scanout across sleep even in the unlocked desktop control.
    virtualisation.qemu.options = [ "-vga virtio" "-nodefaults" "-global virtio-vga.x-pcie-pm-no-soft-reset=on" ];
    virtualisation.memorySize = 4096;
    virtualisation.cores = 2;
    # virtio-fs refuses suspend. Keep the test closure on an ephemeral EROFS
    # disk and use the serial test backdoor instead of shared filesystems.
    virtualisation.useNixStoreImage = true;
    virtualisation.writableStore = true;
    virtualisation.sharedDirectories = lib.mkForce { };
    environment.etc."xdg/kscreenlockerrc".text = ''
      [Daemon]
      Autolock=true
      LockOnResume=true
      Lock=true
      LockGrace=0
      RequirePassword=true
      Timeout=10
    '';
    users.users.observer = {
      isNormalUser = true;
      uid = 1000;
      password = "aB3! z";
    };
    services.xserver.enable = true;
    services.desktopManager.plasma6.enable = true;
    services.displayManager.sddm = {
      enable = true;
      wayland.enable = true;
    };
    services.displayManager.autoLogin = {
      enable = true;
      user = "observer";
    };
    services.displayManager.defaultSession = "plasma";
    programs.kde-pim.enable = false;
    environment.systemPackages = [ pkgs.pamtester ];
    systemd.sleep.settings.Sleep.MemorySleepMode = "s2idle";
    services.logind.settings.Login.HandlePowerKey = "ignore";
    # Keep logind's native Plasma locking and production Gaming pre/post hooks;
    # replace only the kernel backend to inspect the resume password prompt.
    systemd.services.systemd-suspend.serviceConfig.ExecStart = lib.mkForce [ ""
      "${pkgs.writeShellScript "sleep-backend" ''
        touch /run/sleep-backend-entered
      ''}"
    ];

  };
  testScript = builtins.readFile ./test.py;
}
