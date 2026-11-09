{ flake ? builtins.getFlake "git+file://${toString ../../..}"
, inputs ? flake.inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs {
    inherit system;
    config.allowUnfree = true;
    inherit ((import ../../../config/system/nixos/users/ssh-agent-auth.nix {}).nixpkgs) overlays;
  };
  passwordFallback = (import ../../../config/lib/btrc.nix { inherit pkgs inputs; }) {
    name = "sudo-password-fallback";
    src = pkgs.lib.fileset.toSource {
      root = ../../..;
      fileset = pkgs.lib.fileset.unions [ ../../lib ./PasswordFallback.btrc ];
    };
    programs.sudo-password-fallback = "tests/nixosctl/onepassword/PasswordFallback.btrc";
    linkFlags = [ "-lutil" ];
  };
in (import ../../vm/test.nix { inherit pkgs inputs; }) {
  name = "onepassword-integration";
  nodes.machine = { ... }: {
    imports = [
      ../../../config/settings.nix
      ../../../config/pkgs/utils/1password.nix
    ];
    security.polkit.enable = true;
    security.pam.sshAgentAuth.enable = true;
    security.sudo.extraConfig = "Defaults passwd_tries=1";
    environment.systemPackages = [ passwordFallback ];
    programs.zsh.enable = true;
    settings.users.admin = {
      username = "operator";
      homeDirectory = "/srv/operator";
    };
    users.users.operator = {
      isNormalUser = true;
      home = "/srv/operator";
      createHome = true;
      extraGroups = [ "wheel" ];
      initialPassword = "testpass123";
    };
    virtualisation.memorySize = 1024;
  };
  scenario = ./Main.btrc;
}
