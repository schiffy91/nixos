{ system ? builtins.currentSystem, inputs ? (builtins.getFlake "git+file://${toString ../../..}").inputs }:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  input = pkgs.runCommandCC "recovery-test-input" {} ''
    mkdir -p $out/bin
    $CC -Wall -Wextra -Werror ${../../plymouth-keyboard/input.c} -o $out/bin/test-input
  '';
in (import ../../vm/test.nix { inherit pkgs inputs; }) {
  name = "recovery-touch-keyboard";
  nodes.machine = { pkgs, ... }: {
    imports = [ ../../../config/system/nixos/images/touch-console.nix ];
    services.buffyboard.enable = true;
    services.getty.autologinUser = "root";
    boot.kernelParams = [ "nomodeset" "console=tty0" ];
    virtualisation = { useEFIBoot = true; useBootLoader = true; qemu.options = [ "-vga std" ]; };
    boot.loader.systemd-boot.enable = true;
    systemd.services.test-input = {
      serviceConfig.ExecStart = "${input}/bin/test-input touchscreen";
    };
    systemd.services.keyboard-read = {
      conflicts = [ "getty@tty1.service" ];
      serviceConfig = {
        StandardInput = "tty-force";
        StandardOutput = "tty";
        TTYPath = "/dev/tty1";
      };
      script = ''
        read -r value
        printf %s "$value" > /run/touch-typed
      '';
    };
  };
  scenario = ./Main.btrc;
}
