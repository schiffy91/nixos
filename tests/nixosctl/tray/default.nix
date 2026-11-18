{ system ? builtins.currentSystem, inputs ? (builtins.getFlake "git+file://${toString ../../..}").inputs }:
let
  pkgs = import inputs.nixpkgs { inherit system; };
in (import ../../lib/vm-test.nix { inherit pkgs inputs; }) {
  name = "nixosctl-tray";
  nodes.machine = { pkgs, lib, ... }: {
    _module.args.inputs = inputs;
    imports = [ ../../../config/settings.nix ../../../config/pkgs/utils/nixosctl/ui ../../../config/system/nixos/storage.nix ];
    settings.users.admin.username = "observer";
    settings.desktop.enable = true;  # the tray is a desktop's
    settings.apps.utils.nixosctl.enable = true;
    settings.apps.utils.nixosctl.caffeine.enable = false;
    settings.disk.storage.card = { label = "CARD"; mountPoint = "/card"; };  # never attached here
    settings.apps.utils.nixosctl.tray.status.Fixture = {
      run = "cat /run/tray-fixture/line";
      attention = "test -e /run/tray-fixture/attention";
    };
    systemd.tmpfiles.rules = [ "d /run/tray-fixture 0755 observer users -" "f /run/tray-fixture/line 0644 observer users - first" ];
    users.users.observer = { isNormalUser = true; uid = 1000; password = "observer"; };
    services.xserver.enable = true;
    services.desktopManager.plasma6.enable = true;
    services.displayManager.sddm = { enable = true; wayland.enable = true; };
    services.displayManager.autoLogin = { enable = true; user = "observer"; };
    services.displayManager.defaultSession = "plasma";
    programs.kde-pim.enable = false;
    virtualisation.qemu.options = [ "-vga virtio" ];
    virtualisation.memorySize = 4096;
    virtualisation.cores = 2;
  };
  scenario = ./Main.btrc;
}
