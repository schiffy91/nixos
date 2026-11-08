{ flake ? builtins.getFlake "git+file://${toString ../../..}"
, inputs ? flake.inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
in pkgs.testers.runNixOSTest {
  name = "sunshine-credentials";
  node.pkgsReadOnly = false;
  nodes.machine = { config, lib, pkgs, ... }: {
    _module.args.inputs = inputs;
    imports = [
      (inputs.nixpkgs + "/nixos/tests/common/x11.nix")
      ../../../config/settings.nix
      ../../../config/pkgs/gaming/sunshine/default.nix
    ];
    settings.users.admin = { username = "operator"; homeDirectory = "/srv/operator"; };
    users.users.operator = { isNormalUser = true; home = "/srv/operator"; uid = 1000; extraGroups = [ "video" "audio" "input" ]; };
    test-support.displayManager.auto.user = "operator";
    services.sunshine = {
      autoStart = lib.mkForce false;
      capSysAdmin = lib.mkForce false;
      settings = {
        capture = lib.mkForce "x11";
        encoder = "software";
        system_tray = "disabled";
      };
    };
    systemd.user.services.sunshine.environment = { DISPLAY = ":0"; XAUTHORITY = "/srv/operator/.Xauthority"; };
    environment.systemPackages = [
      pkgs.python3 pkgs.moonlight-qt
      (import ../../../config/pkgs/utils/nixosctl/package.nix {
        inherit pkgs lib;
        btrcpy = inputs.btrc.packages.${system}.btrcpy;
      })
    ];
    environment.etc."sunshine-credentials-test.py".source = ./test.py;
    virtualisation.memorySize = 4096;
    virtualisation.cores = 4;
  };
  testScript = ''
    start_all()
    machine.wait_for_x()
    machine.wait_for_unit("user@1000.service")
    machine.succeed("python3 /etc/sunshine-credentials-test.py", timeout=300)
    machine.shutdown()
    machine.start()
    machine.wait_for_x()
    machine.wait_for_unit("user@1000.service")
    machine.succeed("python3 /etc/sunshine-credentials-test.py reboot", timeout=120)
  '';
}
