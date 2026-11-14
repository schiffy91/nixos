{ inputs ? (builtins.getFlake "git+file://${toString ../../..}").inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  programs = import ../../../config/pkgs/utils/drive/package.nix { inherit pkgs inputs; };
  # A desktop session reduced to what the question needs: a compositor, and its display
  # known to the user's service manager, as a desktop's is.
  session = pkgs.writeShellScript "drive-test-session" ''
    ${pkgs.systemd}/bin/systemctl --user import-environment WAYLAND_DISPLAY
    exec ${pkgs.coreutils}/bin/sleep infinity
  '';
in (import ../../vm/test.nix { inherit pkgs inputs; }) {
  name = "drive-dialog";
  graphical = true;
  nodes.machine = { lib, ... }: {
    boot.kernelModules = [ "fuse" ];
    users.users.fixture = { isNormalUser = true; uid = 1000; };
    services.cage = {
      enable = true;
      user = "fixture";
      program = session;
      environment = { WLR_RENDERER = "pixman"; WLR_NO_HARDWARE_CURSORS = "1"; };
    };
    hardware.graphics.enable = true;  # the window draws through Vulkan; the guest has only the software driver
    fonts.packages = [ pkgs.dejavu_fonts ];
    environment.systemPackages = [ programs.drive programs.fake programs.askSession pkgs.curl pkgs.attr pkgs.python3 ];
    virtualisation.qemu.options = [ "-vga std" "-vnc unix:screen.vnc" ];
    virtualisation.memorySize = 4096;
    virtualisation.cores = 4;
  };
  scenario = ./Main.btrc;
  extraFiles = [ ../Fixture.btrc ./btrc.toml ];
}
