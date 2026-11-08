{ flake ? builtins.getFlake "git+file://${toString ../../..}"
, inputs ? flake.inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; config.allowUnfree = true; };
in pkgs.testers.runNixOSTest {
  name = "onepassword-integration";
  nodes.machine = { ... }: {
    imports = [ ../../../config/settings.nix ../../../config/pkgs/utils/1password.nix ];
    security.polkit.enable = true;
    settings.users.admin = {
      username = "operator";
      homeDirectory = "/srv/operator";
    };
    users.users.operator = {
      isNormalUser = true;
      home = "/srv/operator";
      createHome = true;
    };
    virtualisation.memorySize = 1024;
  };
  testScript = ''
    start_all()
    machine.wait_for_unit("multi-user.target")
    machine.succeed("runuser -u operator -- op --version")
    machine.succeed("test $(stat -c %U:%G:%a /run/wrappers/bin/op) = root:onepassword-cli:2511")
    machine.succeed("test $(stat -c %U:%G:%a /run/wrappers/bin/1Password-BrowserSupport) = root:onepassword:2511")
    machine.succeed("test $(stat -c %U:%a /srv/operator/.config/1Password/ssh/agent.toml) = operator:600")
    machine.succeed("runuser -u operator -- grep -F 'vault = \"Private\"' /srv/operator/.config/1Password/ssh/agent.toml")
    machine.fail("test -e /home/operator/.config/1Password")
    machine.succeed("grep -F unix-user:operator /run/current-system/sw/share/polkit-1/actions/com.1password.1Password.policy")
    machine.reboot()
    machine.wait_for_shutdown()
    machine.start()
    machine.wait_for_unit("multi-user.target")
    machine.succeed("runuser -u operator -- op --version")
    machine.succeed("test $(stat -c %U:%G:%a /run/wrappers/bin/op) = root:onepassword-cli:2511")
  '';
}
